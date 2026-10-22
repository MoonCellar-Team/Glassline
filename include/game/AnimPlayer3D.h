#pragma once

#include "runtime/midp/Object.h"

class AnimationManager3D;
namespace midp {
class Class;
}
namespace m3g {
class Node;
}

class AnimPlayer3D : public midp::Object {
public:
    static const int START_LOOP = 4;
    static const int START_RESTART = 16;

    static const int FLAG_VALID = 1;
    static const int FLAG_ANIMATING = 2;
    static const int FLAG_LOOP = 4;
    static const int FLAG_REVERSE = 8;

    explicit AnimPlayer3D(AnimationManager3D& manager);
    virtual ~AnimPlayer3D();
    virtual midp::Class* getClass() const;

    void setNode(m3g::Node* node);
    void startAnim(int anim, int flags);
    void updateAnim(int dt);
    bool isAnimating();
    void setAnimating(bool animating);
    int getAnimID();
    int getAnimTime() const;
    void setAnimTime(int time);
    int getAnimDuration() const;

    AnimationManager3D* m_manager;
    int m_animID;
    int m_frameTime;
    int m_time;
    int m_startTime;
    int m_endTime;
    int m_speed;
    int m_flags;
    m3g::Node* m_node;
};
