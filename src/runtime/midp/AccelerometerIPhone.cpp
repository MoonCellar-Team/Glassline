#include "runtime/midp/AccelerometerIPhone.h"

#include <stddef.h>

namespace midp {

AccelerometerIPhone* AccelerometerIPhone::s_accelerometer;

void AccelerometerIPhone::accelerate(float x, float y, float z) {
    float keep = 1.0f - m_filter;
    m_x = m_x * keep + m_filter * x;
    m_y = m_y * keep + m_filter * y;
    m_z = m_z * keep + m_filter * z;
    m_rawX = x;
    m_rawY = y;
    m_rawZ = z;
}

void AccelerometerIPhone::setFilterFactor(float factor) {
    m_filter = factor;
}

void AccelerometerIPhone::getRawXYZ(float* x, float* y, float* z) {
    *x = m_rawX;
    *y = m_rawY;
    *z = m_rawZ;
}

AccelerometerIPhone::AccelerometerIPhone()
    : m_objc(NULL), m_filter(1.0f), m_rawX(0), m_rawY(0), m_rawZ(0), m_x(0), m_y(0), m_z(0) {
    m_objc = iPhoneAccelerometer::alloc()->initWithCppInterface(this);
}

AccelerometerIPhone::~AccelerometerIPhone() {
    if (m_objc != NULL)
        m_objc->release();
}

AccelerometerIPhone* AccelerometerIPhone::getStaticAccelerometer() {
    if (s_accelerometer == NULL)
        s_accelerometer = new AccelerometerIPhone();
    return s_accelerometer;
}

}
