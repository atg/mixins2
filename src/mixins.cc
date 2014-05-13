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

		/*Int32 msgid = 0;
		CFDataRef outData = NULL;
		CFTimeInterval sendTimeout = 1.0;
		CFTimeInterval rcvTimeout = sendTimeout;

		SInt32 returnCode = 0;
		SInt32* returnCodePtr = &returnCode;

		dispatch_block_t block = ^{
			// Don't do any node stuff in this block without a HandleScope
			// Preprare everything OUTSIDE the block
			SInt32 code = CFMessagePortSendRequest(remote, msgid, data, sendTimeout, rcvTimeout, replyMode, &outData);
		};

		dispatch_sync(msg_queue, block);
		
		// Deserialize outData*/
		NanReturnUndefined();
	}

	void InitMixins(Handle<Object> target) {
		NanScope();
		
		msg_queue = dispatch_queue_create(NULL, NULL);
		
		dispatch_async(msg_queue, ^{
			local = CFMessagePortCreateLocal(NULL, CFSTR("chocolat-to-js"), chocolat_callback, NULL, NULL);
			remote = CFMessagePortCreateRemote(NULL, CFSTR("js-to-chocolat"));
			
			CFMessagePortSetDispatchQueue(local, msg_queue);
		});
		
		NODE_SET_METHOD(target, "objc_msgSend", ObjCMsgSend);
	}
}