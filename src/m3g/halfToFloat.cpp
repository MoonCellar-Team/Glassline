#include "m3g/halfToFloat.h"

namespace m3g {

float halfToFloat::operator()(short value) {
    unsigned bits = static_cast<unsigned short>(value);
    unsigned exponent = (bits >> 10) & 31;
    float fraction = static_cast<float>(bits & 1023) * 0.0009765625f;
    if (exponent != 0) fraction += 1.0f;
    float sign = value < 1 ? 1.0f : -1.0f;
    int power = static_cast<int>(1u << exponent);
    return ((sign * fraction) * static_cast<float>(power)) * 0.000030517578125f;
}

}
