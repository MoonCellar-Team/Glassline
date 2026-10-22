#pragma once
#include "platform/AppleSDK.h"
#include "platform/QueueObject.h"

@interface AudioQueueAction : NSObject
- (void)pauseQueue:(QueueObject*)object;
- (void)startQueue:(QueueObject*)object;
@end
