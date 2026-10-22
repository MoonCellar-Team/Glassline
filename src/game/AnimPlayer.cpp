#include "game/AnimPlayer.h"

#include "game/AnimationManager.h"

AnimPlayer::AnimPlayer() : m_anim(-1), m_frame(0), m_frameTime(0), m_flags(0) {}

void AnimPlayer::startAnim(int anim, int flags) {
    if (!(flags & START_RESTART) && m_anim == anim) {
        if (m_frame >= AnimationManager::getAnimFrameCount(m_anim)) {
            m_frame = 0;
        } else if (m_frame < 0) {
            m_frame = AnimationManager::getAnimFrameCount(m_anim) - 1;
        }
    } else {
        m_anim = anim;
        if (m_flags & FLAG_REVERSE) {
            m_frame = AnimationManager::getAnimFrameCount(anim) - 1;
        } else {
            m_frame = 0;
        }
        m_frameTime = 0;
    }
    m_flags = (flags & START_LOOP) | FLAG_VALID | FLAG_ANIMATING;
}

void AnimPlayer::prevFrame() {
    int time = m_frameTime - AnimationManager::getAnimFrameDuration(m_anim, m_frame);
    if (time < 0) {
        time = 0;
    }
    m_frameTime = time;
    m_frame--;
    if (m_frame > 0) {
        return;
    }
    if (m_flags & FLAG_LOOP) {
        m_frame = AnimationManager::getAnimFrameCount(m_anim) - 1;
    } else {
        m_frame = 0;
        m_flags &= ~FLAG_ANIMATING;
    }
}

void AnimPlayer::nextFrame() {
    int time = m_frameTime - AnimationManager::getAnimFrameDuration(m_anim, m_frame);
    if (time < 0) {
        time = 0;
    }
    m_frameTime = time;
    m_frame++;
    if (m_frame < AnimationManager::getAnimFrameCount(m_anim)) {
        return;
    }
    if (m_flags & FLAG_LOOP) {
        m_frame = 0;
    } else {
        m_frame = AnimationManager::getAnimFrameCount(m_anim) - 1;
        m_flags &= ~FLAG_ANIMATING;
    }
}

bool AnimPlayer::updateAnim(int dt) {
    if (!(m_flags & FLAG_ANIMATING)) {
        return false;
    }
    m_frameTime += dt;
    if (m_frameTime <= AnimationManager::getAnimFrameDuration(m_anim, m_frame)) {
        return false;
    }
    if (m_flags & FLAG_REVERSE) {
        prevFrame();
    } else {
        nextFrame();
    }
    return true;
}

void AnimPlayer::drawAnim(midp::Graphics* g, int x, int y) {
    if (m_anim >= 0) {
        AnimationManager::drawAnimFrame(g, m_anim, m_frame, x, y);
    }
}
