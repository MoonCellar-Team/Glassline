#pragma once
#include "platform/AppleSDK.h"

namespace midp { class AccelerometerIPhone; }

@interface iPhoneAccelerometer : NSObject {
@public
    midp::AccelerometerIPhone* m_accelerometer;
}
- (id)initWithCppInterface:(midp::AccelerometerIPhone*)accelerometer;
@end
