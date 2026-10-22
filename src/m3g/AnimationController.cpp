#include "m3g/AnimationController.h"

namespace m3g {

AnimationController::AnimationController() : m_activeStart(0), m_activeEnd(0), m_weight(1.0f), m_speed(1.0f), m_position(0.0f), m_positionTime(0) {}

AnimationController::~AnimationController() {}

void AnimationController::updateAnimationProperty(int, const midp::array<float>&) {}

void AnimationController::updateAnimationProperty(int, const midp::array<int>&) {}

float AnimationController::getPosition(int time) {
    int delta = static_cast<int>(static_cast<unsigned>(time) - static_cast<unsigned>(m_positionTime));
    return m_position + m_speed * static_cast<float>(delta);
}

void AnimationController::setActiveInterval(int start, int end) { m_activeStart = start; m_activeEnd = end; }

void AnimationController::setPosition(float position, int time) { m_position = position; m_positionTime = time; }

void AnimationController::setSpeed(float speed, int time) { float position = getPosition(time); m_speed = speed; m_position = position; m_positionTime = time; }

AnimationController* AnimationController::duplicate() {
    AnimationController* result = new AnimationController;
    duplicateTo(result);
    result->m_activeStart = m_activeStart; result->m_activeEnd = m_activeEnd;
    result->m_weight = m_weight; result->m_speed = m_speed;
    result->m_position = m_position; result->m_positionTime = m_positionTime;
    return result;
}

int AnimationController::getM3GUniqueClassID() const { return 1; }

float AnimationController::getWeight() const { return m_weight; }

void AnimationController::setWeight(float weight) { m_weight = weight; }

bool AnimationController::isZeroWeight() const {
    unsigned bits; __builtin_memcpy(&bits, &m_weight, sizeof(bits));
    return (bits & 0x60000000u) == 0;
}

}
