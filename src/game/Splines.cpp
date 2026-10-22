#include "game/Splines.h"

#include "game/GameCommon.h"

static float cubic(float p0, float m0, float p1, float m1, float t, float t2, float t3) {
    float a = (p0 + p0) + (p1 * -2.0f + m0) + m1;
    float b = ((p0 * -3.0f + p1 * 3.0f) + m0 * -2.0f) - m1;
    return t3 * a + t2 * b + m0 * t + p0;
}

MathVector Splines::getPositionOnCubic(const MathVector& p0, const MathVector& m0, const MathVector& p1,
                                       const MathVector& m1, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return MathVector(cubic(p0.x, m0.x, p1.x, m1.x, t, t2, t3), cubic(p0.y, m0.y, p1.y, m1.y, t, t2, t3),
                      cubic(p0.z, m0.z, p1.z, m1.z, t, t2, t3));
}
