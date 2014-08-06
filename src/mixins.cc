/*
 * Copyright (c) 2014, Nicholas Penree <nick@penree.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#import <CoreFoundation/CFMessagePort.h>

#include <node.h>
#include <nan.h>
#include "./mixins.h"

using namespace node;
using namespace v8;

namespace mixins {
	
	Persistent<Function> handler;
	static CFMessagePortRef local;
	static CFMessagePortRef remote;
	static CFRunLoopSourceRef rls;
	static dispatch_queue_t msg_queue;
	
	static void ChocolatPollRunLoop_Idle(uv_idle_t* handle, int status) {
//		printf("[node2] Polling run IDLE loop\n");
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, false);
	}
	
	static void ChocolatPollRunLoop_Check(uv_check_t* handle, int status) {
//		printf("[node2] Polling run CHECK loop\n");
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, false);
	}
	
	
	static CFDataRef chocolat_callback(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void *info) {
		NanScope();

		// Input
//		printf("[node2] Chocolat callback\n");
//		CFShow(data);
//		printf("[node2] /end data\n");
		
		/*
		 
		 <CFData 0x60800005c470 [0x7fff71d90f00]>{length = 96, capacity = 96, bytes = 0x7b22746f6b656e223a224e4f44445924 ... 726773223a5b5d7d}
		 [node2] /end data
		 Assertion failed: (val->IsString()), function _NanGetExternalParts, file ../node_modules/nan/nan.h, line 1725.
		 
		 */
		
//		printf("[node2] A\n");
		const char* by = reinterpret_cast<const char*>(CFDataGetBytePtr(data));
//		printf("[node2] B\n");
		Handle<String> json = v8::String::New(by);
//		printf("[node2] C\n");
		Handle<Object> that = Object::New();
//		printf("[node2] D\n");
		Handle<Value> argv[] = { json };
//		printf("[node2] E\n");
		
		// Call
		Handle<Value> ret = handler->Call(that, sizeof(argv) / sizeof(*argv), argv);
//		printf("[node2] F\n");
		
		// Output
		size_t len;
		char* jsonText = NanCString(ret, &len);
//		printf("[node2] G\n");
		// libc++abi.dylib: terminating with uncaught exception of type std::logic_error: basic_string::_S_construct NULL not valid

		CFDataRef outData = CFDataCreate(NULL, (const UInt8 *)jsonText, len);
//		printf("[node2] outData\n");
//		CFShow(outData);
//		printf("[node2] /end outData\n");
		delete[] jsonText;

		return outData;
	}
	
	NAN_METHOD(ChocolatParentIsGone) {
		NanScope();
		if (getppid() <= 1)
			NanReturnValue(NanTrue());
		
		NanReturnValue(NanFalse());
	}
	
	NAN_METHOD(ChocolatSendJSON) {
		NanScope();
		
		CFTimeInterval sendTimeout = 1.0;
		CFTimeInterval rcvTimeout = sendTimeout;
		
		size_t len;
		char* json = NanCString(args[0], &len);
		
		CFDataRef data = CFDataCreate(NULL, (const UInt8 *)json, len);
		delete[] json;
		
		CFDataRef outData = NULL;
		SInt32 msgid = 0;
		SInt32 returnCode = CFMessagePortSendRequest(remote, msgid, data, sendTimeout, rcvTimeout, kCFRunLoopDefaultMode, &outData);
				
		// Deserialize outData
		if (outData) {
			const char* by = reinterpret_cast<const char*>(CFDataGetBytePtr(outData));
			Handle<String> out = v8::String::New(by);
		
			CFRelease(data);
			CFRelease(outData);
		
			NanReturnValue(NanNew<String>(out));
		}
		else {
			CFRelease(data);
			NanReturnValue(NanUndefined());
		}
	}
	
	NAN_METHOD(ChocolatRegisterFunctionHandler) {
		NanScope();
		
		// Provides a callback
		handler = Persistent<Function>::New(args[0].As<Function>());
		
		NanReturnValue(NanUndefined());
	}

	void InitMixins(Handle<Object> target) {
		NanScope();
		
		// Construct message port names
		const char* mixinID = getenv("CHOCOLAT_MIXIN_ID");
		CFStringRef localName = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.chocolat.mixins2.client.%s"), mixinID);
		CFStringRef remoteName = CFStringCreateWithFormat(NULL, NULL, CFSTR("com.chocolat.mixins2.server.%s"), mixinID);
		
//		printf("NODDY2 IS STARTING LOCAL: %s\n", mixinID);
//		CFShow(localName);
//		printf("I MEAN IT!\n");
		
		// Create message ports
		local = CFMessagePortCreateLocal(NULL, localName, chocolat_callback, NULL, NULL);
		remote = CFMessagePortCreateRemote(NULL, remoteName);
		
//		msg_queue = dispatch_queue_create(NULL, NULL);
//		CFMessagePortSetDispatchQueue(local, msg_queue);
		
		// Set up functions in v8
		NODE_SET_METHOD(target, "chocolat_sendJSON", ChocolatSendJSON);
		NODE_SET_METHOD(target, "chocolat_registerFunctionHandler", ChocolatRegisterFunctionHandler);
		NODE_SET_METHOD(target, "chocolat_parentIsGone", ChocolatParentIsGone);
		
		// Add the local port to the run loop
		rls = CFMessagePortCreateRunLoopSource(NULL, local, 0);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode);
		
		// Set up a uv_prepare to run the run loop on every UV run loop
//		static uv_prepare_t noddy_mq_prep;
//		uv_prepare_init(uv_default_loop(), &noddy_mq_prep);
//		uv_prepare_start(&noddy_mq_prep, ChocolatPollRunLoop);
		
		static uv_idle_t noddy_idle;
		uv_idle_init(uv_default_loop(), &noddy_idle);
		uv_idle_start(&noddy_idle, ChocolatPollRunLoop_Idle);

		static uv_check_t noddy_check;
		uv_check_init(uv_default_loop(), &noddy_check);
		uv_check_start(&noddy_check, ChocolatPollRunLoop_Check);
		
		// Clean up
		CFRelease(localName);
		CFRelease(remoteName);
	}
}