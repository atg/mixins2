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

#define LOCAL_NAME "com.chocolat.mixins2.client"
#define REMOTE_NAME "com.chocolat.mixins2.server"

using namespace node;
using namespace v8;

namespace mixins {
	
	static CFMessagePortRef local;
	static CFMessagePortRef remote;
	static dispatch_queue_t msg_queue;

	static CFDataRef chocolat_callback(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void *info) {
		// Deserialize data
		
		// Serialize a response
		return NULL;
	}

	NAN_METHOD(ObjCMsgSend) {
		NanScope();

		SInt32 msgid = 0;
		UInt8 bytes[] = { 0, 1 };
		char *message = "Ring, ring.";
		CFDataRef data = CFDataCreate(NULL, (const UInt8 *)message, strlen(message)+1);
		CFDataRef outData = NULL;
		CFTimeInterval sendTimeout = 1.0;
		CFTimeInterval rcvTimeout = sendTimeout;

		SInt32 returnCode = CFMessagePortSendRequest(remote, msgid, data, sendTimeout, rcvTimeout, kCFRunLoopDefaultMode, &outData);
		
		// Deserialize outData*/
		const char* by = reinterpret_cast<const char*>(CFDataGetBytePtr(outData));
		Handle<String> out = v8::String::New(by);
		
		NanReturnValue(NanNew<String>(out));
	}

	void InitMixins(Handle<Object> target) {
		NanScope();
		
		msg_queue = dispatch_queue_create(NULL, NULL);
		local = CFMessagePortCreateLocal(NULL, CFSTR(LOCAL_NAME), chocolat_callback, NULL, NULL);
		remote = CFMessagePortCreateRemote(NULL, CFSTR(REMOTE_NAME));
		
		CFMessagePortSetDispatchQueue(local, msg_queue);
		
		NODE_SET_METHOD(target, "objc_msgSend", ObjCMsgSend);
	}
}