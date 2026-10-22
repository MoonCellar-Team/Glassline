#pragma once

#include "m3g/Transformable.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}
namespace util {
class JRandom;
}

namespace particles {

class EmissionMode : public m3g::Transformable {
public:
    EmissionMode();
    virtual ~EmissionMode();
    virtual int getM3GUniqueClassID() const;

    void setRate(float rate);
    void setSpeed(float speed, float deviation);
    void setSpreadAngle(float angle, float deviation);
    void setAcceleration(const float* acceleration);
    void getParticleStartPosition(util::JRandom* random, float* position) const;
    void getParticleStartVelocity(util::JRandom* random, float* velocity) const;
    float getSpeed() const;
    float getSpeedDeviation() const;
    float getSpreadAngle() const;
    float getSpreadAngleDeviation() const;
    float getRate() const;
    const float* getAcceleration() const;

    midp::ReferenceCountedPointer<midp::String> m_unkE4;
    float m_rate;
    float m_speed;
    float m_speedDeviation;
    float* m_acceleration;
    float m_spreadAngle;
    float m_spreadAngleDeviation;
};

}
