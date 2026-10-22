#pragma once

namespace midp {

class Math {
public:
    static const float PI;

    static int abs(int a);
    static float abs(float a);
    static int min(int a, int b);
    static float min(float a, float b);
    static int max(int a, int b);
    static long max(long a, long b);
    static float max(float a, float b);
    static float sin(float a);
    static float cos(float a);
    static float tan(float a);
    static double tan(double a);
    static float sqrt(float a);
    static float toDegrees(float a);
    static float toRadians(float a);
};

}
