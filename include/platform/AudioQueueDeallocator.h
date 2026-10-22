#pragma once
#include "platform/AppleSDK.h"
#include "platform/AudioQueueStructObject.h"

@interface AudioQueueDeallocator : NSObject
- (void)deallocateQueue:(AudioQueueStructObject*)object;
@end
