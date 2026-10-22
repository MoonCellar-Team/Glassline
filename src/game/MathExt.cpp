#include "game/MathExt.h"

#include <math.h>

int MathExt::Fmul(int a, int b) {
    return (int)(((long long)a * b) >> 16);
}

int MathExt::Fdiv(int a, int b) {
    return (int)(((long long)a << 16) / b);
}

float MathExt::atan2(float y, float x) {
    return (float)::atan2((double)y, (double)x);
}
