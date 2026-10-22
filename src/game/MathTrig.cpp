#include "game/MathTrig.h"

#include "game/MathExt.h"
#include "runtime/midp/Math.h"

float MathTrig::calcAngleFromUpToVectorDeg(float x, float y) {
    return MathExt::atan2(x, -y) * -180.0f / midp::Math::PI;
}

void MathTrig::convertLookVectorToEulerRotationsDeg(float x, float y, float z, float& yawDeg,
                                                    float& pitchDeg) {
    float horizontal = midp::Math::sqrt(x * x + z * z);
    yawDeg = midp::Math::toDegrees(MathExt::atan2(-x, -z));
    pitchDeg = midp::Math::toDegrees(MathExt::atan2(y, horizontal));
}
