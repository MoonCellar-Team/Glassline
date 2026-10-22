#pragma once
#include "m3g/Object3D.h"

namespace m3g {

class AnimationController : public Object3D {
public:
    AnimationController();
    virtual ~AnimationController();
    virtual void updateAnimationProperty(int property, const midp::array<float>& value);
    virtual void updateAnimationProperty(int property, const midp::array<int>& value);
    float getPosition(int time);
    void setActiveInterval(int start, int end);
    void setPosition(float position, int time);
    void setSpeed(float speed, int time);
    AnimationController* duplicate();
    virtual int getM3GUniqueClassID() const;
    float getWeight() const;
    void setWeight(float weight);
    bool isZeroWeight() const;
    int m_activeStart;
    int m_activeEnd;
    float m_weight;
    float m_speed;
    float m_position;
    int m_positionTime;
};

}
