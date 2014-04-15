//  Created by Alex Gordon on 27/12/2013.

#import <Foundation/Foundation.h>
#import <CoreFoundation/CFMessagePort.h>

#include <node.h>
#include <v8.h>

// SEE DOCUMENTATION: http://nodejs.org/api/addons.html#addons_hello_world

using namespace v8;

static CFDataRef chocolat_callback(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void *info) {
    // Deserialize data
    
    // Serialize a response
    return NULL;
}


/* implement as
 
    function objc_msgSend(isAsync, isThread, receiver, selector, etc) {
        var args = arguments.slice(2);
        var obj = {
            "sync": !isAsync,
            "thread": isThread,
            "receiver": receiver,
            "selector": selector,
            "args": args,
        };
        return native_objc_msgSend(true, false, JSON.stringify(obj));
    }
*/

static CFMessagePortRef local;
static CFMessagePortRef remote;
static dispatch_queue_t msg_queue;

Handle<Value> native_objc_msgSend(const Arguments& args) {
    HandleScope scope;
    
    bool isAsync = false;
    bool isThread = false;
    
    SInt32 msgid = 0;
    CFDataRef outData = NULL;
    CFDataRef* outDataPtr = isAsync ? NULL : &outData;
    
    // Serialize arguments
    CFTimeInterval sendTimeout = 1.0;
    CFTimeInterval rcvTimeout = isAsync ? 0 : sendTimeout;
    
    SInt32 returnCode = 0;
    SInt32* returnCodePtr = isAsync ? NULL : &returnCode;
    
    dispatch_block_t block = ^{
        // Don't do any node stuff in this block without a HandleScope
        // Preprare everything OUTSIDE the block
        SInt32 code = CFMessagePortSendRequest(remote, msgid, data, sendTimeout, rcvTimeout, replyMode, outDataPtr);
        
        if (isAsync)
            *returnCodePtr = code;
    };
    
    
    if (isAsync) {
        dispatch_async(msg_queue, block);
        return scope.Close(Undefined::New());
    }
    else {
        dispatch_sync(msg_queue, block);
        
        // Deserialize outData
        // ...
        return ...;
    }
}

void init(Handle<Object> exports) {
    msg_queue = dispatch_queue_create(NULL, NULL);
    
    dispatch_async(msg_queue, ^{
        local = CFMessagePortCreateLocal(NULL, CFSTR("chocolat-to-js"), chocolat_callback, NULL, NULL);
        remote = CFMessagePortCreateRemote(NULL, CFSTR("js-to-chocolat"));
        
        CFMessagePortSetDispatchQueue(local, msg_queue);
    });
    
    exports->Set(String::NewSymbol("objc_msgSend"),
      FunctionTemplate::New(chocolat_objc_msgSend)->GetFunction());
}

NODE_MODULE(chocolat, init)
