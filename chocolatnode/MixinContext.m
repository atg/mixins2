//  Created by Alex Gordon on 27/12/2013.

#import "MixinContext.h"

static CFDataRef serialize(id val) {
//    ...
    return NULL;
}

static id deserialize(CFDataRef data) {
//    ...
    return nil;
}

static CFDataRef undefined_placeholder() {
    return ...;
}

//static id target_for_receiver(NSString* receiver) {
//    return nil; // this function shouldn't exist since deserialize should do it automatically
//}

static CFDataRef chocolat_callback(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void *info) {
    // Deserialize data
    NSDictionary* msg = deserialize(data);
    
    // Do something
    Local<Object> thisObject = args.This();
    BOOL isSync = [[msg objectForKey:@"sync"] boolValue];
    BOOL isThread = [[msg objectForKey:@"thread"] boolValue];
    NSArray* args = [msg objectForKey:@"args"];
    
    // Determine the target
    id receiver = args[0];
    if ([target isKindOfClass:[NSString class]] && [target isEqual:@"controller"]) {
        target = [NoddyController sharedController];
    }
    else if (![target respondsToSelector:@selector(noddyID)]) {
        CHDebug(@"Error target %@ does not have a noddyID", target);
        return undefined_placeholder();
    }
    
    NSString* selectorstring = args[1];
    
    // How many objects do we have?
    // Delete all colons to find out how many it has. E.g. [@"a:b:c:" length] - [@"abc" length] == 3
    long numberOfParameters = [selectorstring length] - [[selectorstring stringByReplacingOccurrencesOfString:@":" withString:@""] length];
    
    // Get the parameters
    NSMethodSignature *signature = [target methodSignatureForSelector:NSSelectorFromString(selectorstring)];
    if (!signature) {
        CHDebug(@"'%@' does not respond to @selector(%@)", target, selectorstring);
    }
    
    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
    
    dispatch_block_t invocationBlock = ^{
        [invocation setTarget:target];
        [invocation setSelector:NSSelectorFromString(selectorstring)];
        
        for (long i = 0; i < numberOfParameters; i++) {
            
            const char* signatureType = [signature getArgumentTypeAtIndex:i + 2];
            if (strcmp(signatureType, @encode(id)) == 0 || strcmp(signatureType, @encode(Class)) == 0) {
                
                id argObj = [args objectAtIndex:i];
                if (argObj == [NSNull null])
                    argObj = nil;
                
                [invocation setArgument:&argObj atIndex:i + 2];
            }
            else if ([[args objectAtIndex:i] isKindOfClass:[NSValue class]]) {
                
                NSValue* value = [args objectAtIndex:i];
                const char* valueType = [value objCType];
                if (strcmp(signatureType, valueType) == 0) {
                    
                    NSUInteger alignSize = 128;
                    NSGetSizeAndAlignment(valueType, NULL, &alignSize);
                    char* buffer = new char[alignSize];
                    
                    [value getValue:buffer];
                    [invocation setArgument:buffer atIndex:i + 2];
                    
                    delete buffer;
                }
            }
        }
        
        // Send the message!
        [invocation invoke];
    };
    
    if (isSync) {
        
        I have no idea how to make this threading work in a multiprocess model
        if (isThread) {
            
            // Clear the main queue so that we definitely execute after everything before us
            dispatch_sync(dispatch_get_main_queue(), ^{});
            
            // Now run the invocation on the NODDY thread
            // Since nothing else references the recipe, and the recipe only changes its internal state
            // ...it's ok to temporarily run on a different thread
            invocationBlock();
        }
        else {
            dispatch_sync(dispatch_get_main_queue(), invocationBlock);
        }
        
        if (![signature methodReturnLength])
            return Local<Primitive>::New(Undefined());
        
        id returnvalue = nil;
        
        const char* numberTypes[] = {"c", "i", "s", "l", "q", "C", "I", "S", "L", "Q", "f", "d", "B"};
        
        const char* signatureType = [signature methodReturnType];
        if (strcmp(signatureType, @encode(id)) == 0 || strcmp(signatureType, @encode(Class)) == 0) {
            
            [invocation getReturnValue:&returnvalue];
        }
        else { // Pack into an NSValue
            
            BOOL foundNumber = NO;
            
#define BRIDGE_RET(type, methodname) \
if (0 == strcmp(@encode(type), signatureType)) { \
type val; \
[invocation getReturnValue:&val]; \
returnvalue = [NSNumber methodname:val]; \
foundNumber = YES; \
}
            BRIDGE_RET(float, numberWithFloat)
            BRIDGE_RET(double, numberWithDouble)
            BRIDGE_RET(BOOL, numberWithBool)
            BRIDGE_RET(char, numberWithChar)
            BRIDGE_RET(short, numberWithShort)
            BRIDGE_RET(int, numberWithInt)
            BRIDGE_RET(long, numberWithLong)
            BRIDGE_RET(long long, numberWithLongLong)
            BRIDGE_RET(unsigned char, numberWithUnsignedChar)
            BRIDGE_RET(unsigned short, numberWithUnsignedShort)
            BRIDGE_RET(unsigned int, numberWithUnsignedInt)
            BRIDGE_RET(unsigned long, numberWithUnsignedLong)
            BRIDGE_RET(unsigned long long, numberWithUnsignedLongLong)
            
            if (!foundNumber) {
                
                NSUInteger alignSize = 128;
                NSGetSizeAndAlignment(signatureType, NULL, &alignSize);
                char* buffer = new char[alignSize];
                
                [invocation getReturnValue:buffer];
                
                Class valueClass = [NSValue class];
                returnvalue = [valueClass valueWithBytes:buffer objCType:signatureType];
                
                delete buffer;
            }
        }
        
        Local<Object> result = cocoa_to_node(returnvalue).As<Object>();
        return scope.Close(result);
    }
    else {
        dispatch_async(dispatch_get_main_queue(), invocationBlock);
        
        return undefined_placeholder();
    }
    
    // Serialize a response
    return NULL;
}

@implementation MixinContext

- (id)init {
    self = [super init];
    if (!self) return nil;
    
    _local = CFMessagePortCreateLocal(NULL, CFSTR("chocolat-to-js"), chocolat_callback, NULL, NULL);
    _remote = CFMessagePortCreateRemote(NULL, CFSTR("js-to-chocolat"));
    _queue = dispatch_queue_create("Mixin queue", NULL);
    
    CFMessagePortSetDispatchQueue(_local, _queue);
    
    return self;
}

- (id)sendMessage:(NSDictionary*)msg {
    SInt32 msgid = 0;
    CFDataRef data = NULL;
    CFTimeInterval sendTimeout = 1;
    CFTimeInterval rcvTimeout = 1;
    CFStringRef replyMode = NULL;
    CFDataRef returnedData = NULL;
    
    SInt32 code = CFMessagePortSendRequest(_remote, msgid, data, sendTimeout, rcvTimeout, replyMode, &returnedData);
    
    if (code == 0 && returnedData)
        return deserialize(returnedData);
    return nil;
}

- (void)cleanUp {
    dispatch_release(_queue);
    if (_local) CFRelease(_local);
    if (_remote) CFRelease(_remote);
}
- (void)finalize {
    [self cleanUp];
    [super finalize];
}
- (void)dealloc {
    [self cleanUp];
}

@end
