#pragma once
#include "platform/AppleSDK.h"
#include "platform/AudioQueueStructObject.h"

@interface AudioQueueCloser : NSObject
- (void)closeQueue:(AudioQueueStructObject*)object;
@end
