#include "runtime/midp/Math.h"

#include <math.h>

namespace midp {

const float Math::PI = 3.14159274f;

int Math::abs(int a) {
    return -a >= a ? -a : a;
}

float Math::abs(float a) {
    return -a > a ? -a : a;
}

int Math::min(int a, int b) {
    return a < b ? a : b;
}

float Math::min(float a, float b) {
    return a < b ? a : b;
}

int Math::max(int a, int b) {
    return a > b ? a : b;
}

long Math::max(long a, long b) {
    return a > b ? a : b;
}

float Math::max(float a, float b) {
    return a > b ? a : b;
}

float Math::sin(float a) {
    return sinf(a);
}

float Math::cos(float a) {
    return cosf(a);
}

float Math::tan(float a) {
    return tanf(a);
}

double Math::tan(double a) {
    return ::tan(a);
}

float Math::sqrt(float a) {
    return sqrtf(a);
}

float Math::toDegrees(float radians) {
    return radians * 180.0f / PI;
}

float Math::toRadians(float degrees) {
    return PI * degrees * (1.0f / 180.0f);
}

}
