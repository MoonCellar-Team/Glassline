#pragma once

class MathTrig {
public:
    static float calcAngleFromUpToVectorDeg(float x, float y);
    static void convertLookVectorToEulerRotationsDeg(float x, float y, float z, float& yawDeg,
                                                     float& pitchDeg);
};
