//
//  TESTAppDelegate.m
//  TestServer
//
//  Created by Alex Gordon on 21/05/2014.
//  Copyright (c) 2014 Curapps. All rights reserved.
//

#import "TESTAppDelegate.h"

CFDataRef myCallBack(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void *info) {
//	char *message = "Thanks for calling!";
	
	NSData* nsdata = (__bridge NSData*)data;
	NSError* err = nil;
	id json = [NSJSONSerialization JSONObjectWithData:nsdata options:0 error:&err];
	if (!json) {
		// Error!
		NSLog(@"JSON PARSE ERROR! '%@' / %@", data, err);
	}
	
//	NSLog(@"JSON: %@", json);
	
	id returnObject = @[@3, @2, @1];
	err = nil;
	NSData* returnData = [NSJSONSerialization dataWithJSONObject:returnObject options:0 error:&err];
	if (!returnData) {
		NSLog(@"JSON DUMP ERROR! %@", err);
	}
	
//	NSMutableData* returnDataZero = [returnData mutableCopy];
//	[returnDataZero increaseLengthBy:1];
	
//	CFDataRef returnData = CFDataCreate(NULL, (const UInt8 *)message, strlen(message)+1);
//	printf("here is our received data: '%s'\n", CFDataGetBytePtr(data));
//	return returnData;  // as stated in header, both data and returnData will be released for us after callback returns
	return CFBridgingRetain(returnData);
}

@implementation TESTAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
		CFMessagePortRef local = CFMessagePortCreateLocal(NULL, CFSTR("com.chocolat.mixins2.server"), myCallBack, NULL, false);
		CFRunLoopSourceRef source = CFMessagePortCreateRunLoopSource(NULL, local, 0);
		CFRunLoopAddSource([[NSRunLoop currentRunLoop] getCFRunLoop], source, kCFRunLoopDefaultMode);
//		CFRunLoopRun();    // will not return as long as message port is still valid and source remains on run loop
		CFRelease(local);
}

@end
