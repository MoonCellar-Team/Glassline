#pragma once

class iPhoneAccelerometer;

namespace midp {

class AccelerometerIPhone {
public:
    AccelerometerIPhone();
    virtual ~AccelerometerIPhone();
    static AccelerometerIPhone* getStaticAccelerometer();
    void accelerate(float x, float y, float z);
    void setFilterFactor(float factor);
    void getRawXYZ(float* x, float* y, float* z);

    static AccelerometerIPhone* s_accelerometer;
    iPhoneAccelerometer* m_objc;
    float m_filter;
    float m_rawX;
    float m_rawY;
    float m_rawZ;
    float m_x;
    float m_y;
    float m_z;
};

}

class iPhoneAccelerometer {
public:
    static iPhoneAccelerometer* alloc();
    iPhoneAccelerometer* initWithCppInterface(midp::AccelerometerIPhone* owner);
    void release();
};
