#include "game/particles/EmissionMode.h"

#include "m3g/Transform.h"
#include "runtime/midp/String.h"
#include "runtime/util/JRandom.h"

namespace particles {

int EmissionMode::getM3GUniqueClassID() const {
    return 0;
}

void EmissionMode::setRate(float rate) {
    m_rate = rate;
}

void EmissionMode::setSpeed(float speed, float deviation) {
    m_speed = speed;
    m_speedDeviation = deviation;
}

void EmissionMode::setSpreadAngle(float angle, float deviation) {
    m_spreadAngle = angle;
    m_spreadAngleDeviation = deviation;
}

EmissionMode::EmissionMode()
    : m_rate(0.0f), m_speed(0.0f), m_speedDeviation(0.0f), m_acceleration(NULL), m_spreadAngle(0.0f),
      m_spreadAngleDeviation(0.0f) {
    setSpeed(1.0f, 0.1f);
    setRate(1.0f);
    setSpreadAngle(10.0f, 10.0f);
    setAcceleration(NULL);
}

EmissionMode::~EmissionMode() {
    setAcceleration(NULL);
}

void EmissionMode::setAcceleration(const float* acceleration) {
    if (acceleration != NULL) {
        if (m_acceleration == NULL) {
            m_acceleration = new float[3];
        }
        m_acceleration[0] = acceleration[0];
        m_acceleration[1] = acceleration[1];
        m_acceleration[2] = acceleration[2];
        return;
    }
    if (m_acceleration != NULL) {
        delete[] m_acceleration;
    }
}

void EmissionMode::getParticleStartPosition(util::JRandom* random, float* position) const {
    position[0] = 0.0f;
    position[1] = 0.0f;
    position[2] = 0.0f;
    position[3] = 1.0f;
}

void EmissionMode::getParticleStartVelocity(util::JRandom* random, float* velocity) const {
    float r = random->nextFloat();
    float speed = getSpeed();
    float deviation = getSpeedDeviation();
    velocity[0] = 0.0f;
    velocity[1] = speed + deviation * r;
    velocity[2] = 0.0f;
    velocity[3] = 0.0f;

    r = random->nextFloat();
    float spread = getSpreadAngle();
    float spreadDeviation = getSpreadAngleDeviation();
    float tilt = (r + r + -1.0f) * spreadDeviation + spread;

    r = random->nextFloat();
    float heading = (r + r + -1.0f) * 180.0f;

    m3g::Transform transform;
    transform.postRotate(heading, 0.0f, 1.0f, 0.0f);
    transform.postRotate(tilt, 1.0f, 0.0f, 0.0f);
    transform.transform(velocity, 4);
}

float EmissionMode::getSpeed() const {
    return m_speed;
}

float EmissionMode::getSpeedDeviation() const {
    return m_speedDeviation;
}

float EmissionMode::getSpreadAngle() const {
    return m_spreadAngle;
}

float EmissionMode::getSpreadAngleDeviation() const {
    return m_spreadAngleDeviation;
}

float EmissionMode::getRate() const {
    return m_rate;
}

const float* EmissionMode::getAcceleration() const {
    return m_acceleration;
}

}
