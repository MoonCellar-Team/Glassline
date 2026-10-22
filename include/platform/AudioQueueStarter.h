#pragma once
#include "platform/AppleSDK.h"
#include "platform/PlayerObject.h"

@interface AudioQueueStarter : NSObject
- (void)startPlayer:(PlayerObject*)object;
@end
