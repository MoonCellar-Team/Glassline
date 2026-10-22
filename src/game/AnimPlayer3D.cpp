#include "game/AnimPlayer3D.h"

#include <stddef.h>

#include "game/AnimPlayer3DClass.h"
#include "game/AnimationManager3D.h"
#include "m3g/Node.h"

midp::Class* AnimPlayer3D::getClass() const {
    return new AnimPlayer3DClass;
}

int AnimPlayer3D::getAnimTime() const {
    if (m_flags & FLAG_VALID) {
        return m_time - m_startTime;
    }
    return 0;
}

int AnimPlayer3D::getAnimDuration() const {
    if (m_flags & FLAG_VALID) {
        return (m_endTime - m_startTime) * 128 / m_speed;
    }
    return 0;
}

void AnimPlayer3D::setAnimating(bool animating) {
    if (animating) {
        m_flags |= FLAG_ANIMATING;
    } else {
        m_flags &= ~FLAG_ANIMATING;
    }
}

void AnimPlayer3D::setNode(m3g::Node* node) {
    midp::INCREF(node);
    midp::DECREF(m_node);
    m_node = node;
}

void AnimPlayer3D::setAnimTime(int time) {
    m_time = m_startTime + time;
    if (m_node != NULL) {
        m_node->animate(m_startTime + time);
    }
}

void AnimPlayer3D::startAnim(int anim, int flags) {
    if (!(m_flags & FLAG_VALID)) {
        return;
    }
    if (anim >= m_manager->getNumAnimations() || anim < 0) {
        return;
    }
    if (!(flags & START_RESTART) && m_animID == anim) {
        if (m_time > m_endTime) {
            m_time = m_endTime;
        } else if (m_time < m_startTime) {
            m_time = m_startTime;
        }
    } else {
        m_animID = (short)anim;
        AnimationManager3D* manager = m_manager;
        int startFrame = manager->getAnimStartFrame(anim);
        int endFrame = manager->getAnimEndFrame(anim);
        m_startTime = m_frameTime * startFrame;
        m_endTime = m_frameTime * endFrame;
        if (m_flags & FLAG_REVERSE) {
            m_time = m_endTime;
        } else {
            m_time = m_startTime;
        }
    }
    m_flags = (flags & START_LOOP) | (m_flags & FLAG_REVERSE) | FLAG_VALID | FLAG_ANIMATING;
    if (m_node != NULL) {
        m_node->animate(m_time);
    }
}

void AnimPlayer3D::updateAnim(int dt) {
    int flags = m_flags;
    if ((flags & (FLAG_VALID | FLAG_ANIMATING)) != (FLAG_VALID | FLAG_ANIMATING)) {
        return;
    }
    int startTime = m_startTime;
    int endTime = m_endTime;
    int step = (m_speed * dt) >> 7;
    int time;
    if (flags & FLAG_REVERSE) {
        time = m_time - step;
        if (time < startTime) {
            if (!(flags & FLAG_LOOP)) {
                flags &= ~FLAG_ANIMATING;
                time = startTime;
            } else if (endTime - startTime > step) {
                time = endTime + startTime + time;
            } else {
                time = endTime;
            }
        }
    } else {
        time = m_time + step;
        if (time > endTime) {
            if (!(flags & FLAG_LOOP)) {
                flags &= ~FLAG_ANIMATING;
                time = endTime;
            } else if (endTime - startTime > step) {
                time = startTime - endTime + time;
            } else {
                time = endTime;
            }
        }
    }
    m_time = time;
    m_flags = flags;
    if (m_node != NULL) {
        m_node->animate(time);
    }
}

AnimPlayer3D::~AnimPlayer3D() {
    midp::DECREF(m_node);
    m_node = NULL;
}

AnimPlayer3D::AnimPlayer3D(AnimationManager3D& manager)
    : m_manager(&manager),
      m_animID(-1),
      m_frameTime(40),
      m_time(0),
      m_startTime(0),
      m_endTime(0),
      m_speed(128),
      m_flags(FLAG_VALID),
      m_node(NULL) {}

bool AnimPlayer3D::isAnimating() {
    return (m_flags & FLAG_ANIMATING) != 0;
}

int AnimPlayer3D::getAnimID() {
    return m_animID;
}
