#pragma once
#include "platform/AppleSDK.h"

namespace util { class Timer; }

@interface CocoaTimer : NSObject {
@public
    NSTimer* runTimer;
    double runInterval;
    util::Timer* timer;
    BOOL onceOnly;
}
- (void)schedule:(util::Timer*)timer withDelay:(int)delay andPeriod:(int)period;
- (void)scheduleOnce:(util::Timer*)timer withDelay:(int)delay;
- (void)setTimer:(util::Timer*)timer;
- (void)run;
@end
