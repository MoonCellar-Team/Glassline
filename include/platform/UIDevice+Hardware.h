#pragma once
#include "platform/AppleSDK.h"
@interface UIDevice (Hardware)
- (NSString*)platform; - (unsigned)platformType; - (unsigned)platformCapabilities;
@end
