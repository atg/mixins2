//  Created by Alex Gordon on 27/12/2013.

#import <Foundation/Foundation.h>
#import <CoreFoundation/CFMessagePort.h>

@interface MixinContext : NSObject

@property dispatch_queue_t queue;
@property CFMessagePortRef local;
@property CFMessagePortRef remote;

@end
