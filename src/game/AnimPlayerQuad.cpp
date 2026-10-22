#include "game/AnimPlayerQuad.h"

#include <stddef.h>

#include "game/MathExt.h"
#include "game/QuadManager.h"
#include "game/QuadMesh.h"

bool AnimPlayerQuad::isAnimating() {
    return m_animating;
}

void AnimPlayerQuad::stopAnim() {
    m_animating = false;
}

bool AnimPlayerQuad::isAnimatingBackward() {
    if (m_animating) {
        return m_reverse;
    }
    return false;
}

bool AnimPlayerQuad::isAnimatingForward() {
    if (m_animating) {
        return !m_reverse;
    }
    return false;
}

AnimPlayerQuad::AnimPlayerQuad(QuadManager* manager, int id)
    : m_manager(manager),
      m_id(id),
      m_keyframe(0),
      m_time(0),
      m_animating(false),
      m_startKeyframe(0),
      m_endKeyframe(0),
      m_loop(false),
      m_reverse(false) {}

int AnimPlayerQuad::getCurrentKeyframe() {
    if (m_time != 0 && m_reverse) {
        return m_keyframe + 1;
    }
    return m_keyframe;
}

void AnimPlayerQuad::playAnimTo(int keyframe) {
    if (m_time == 0 && m_keyframe == keyframe) {
        m_animating = false;
        return;
    }
    m_animating = true;
    m_endKeyframe = keyframe;
    m_loop = false;
    m_reverse = m_keyframe >= keyframe;
    if (m_keyframe >= keyframe && m_time != 0) {
        m_startKeyframe = m_keyframe + 1;
    } else {
        m_startKeyframe = m_keyframe;
    }
}

void AnimPlayerQuad::nextFrame() {
    if (m_keyframe != m_endKeyframe - 1) {
        m_time -= m_keyframeTimes[m_keyframe];
        m_keyframe++;
    } else if (m_loop) {
        m_time -= m_keyframeTimes[m_keyframe];
        m_keyframe = m_startKeyframe;
    } else {
        m_time = 0;
        m_keyframe = m_endKeyframe;
        m_animating = false;
    }
}

void AnimPlayerQuad::previousFrame() {
    if (m_keyframe != m_endKeyframe) {
        m_keyframe--;
        m_time += m_keyframeTimes[m_keyframe];
    } else if (m_loop) {
        m_keyframe = m_startKeyframe - 1;
        m_time += m_keyframeTimes[m_startKeyframe - 1];
    } else {
        m_time = 0;
        m_keyframe = m_endKeyframe;
        m_animating = false;
    }
}

void AnimPlayerQuad::setupAttribute(int attrib, QuadMesh* mesh, int meshAttrib) {
    m_attribs[attrib].m_mesh = mesh;
    m_attribs[attrib].m_attrib = meshAttrib;
}

void AnimPlayerQuad::refreshMeshes() {
    if (m_time == 0) {
        for (int i = m_attribs.m_length; i != 0;) {
            i--;
            QuadAttrib& attrib = m_attribs[i];
            attrib.m_mesh->m_attribs[attrib.m_attrib] = attrib.m_values[m_keyframe];
            attrib.m_mesh->m_dirty = true;
        }
    } else {
        int t = MathExt::Fdiv(m_time, m_keyframeTimes[m_keyframe]);
        for (int i = m_attribs.m_length; i != 0;) {
            i--;
            QuadAttrib& attrib = m_attribs[i];
            int from = attrib.m_values[m_keyframe];
            int to = attrib.m_values[m_keyframe + 1];
            if (from == to) {
                attrib.m_mesh->m_attribs[attrib.m_attrib] = from;
            } else {
                attrib.m_mesh->m_attribs[attrib.m_attrib] = MathExt::Fmul(to - from, t) + from;
            }
            attrib.m_mesh->m_dirty = true;
        }
    }
    for (int i = m_attribs.m_length; i != 0;) {
        i--;
        QuadMesh* mesh = m_attribs[i].m_mesh;
        if (mesh->m_dirty) {
            m_manager->updateMesh(mesh);
        }
    }
}

void AnimPlayerQuad::update(int dt) {
    if (!m_animating) {
        return;
    }
    if (m_reverse) {
        m_time -= dt;
        while (m_animating && m_time < 0) {
            previousFrame();
        }
    } else {
        m_time += dt;
        while (m_animating && m_time >= m_keyframeTimes[m_keyframe]) {
            nextFrame();
        }
    }
    refreshMeshes();
}

void AnimPlayerQuad::snapToTime(int keyframe, int time) {
    int startKeyframe = m_startKeyframe;
    int endKeyframe = m_endKeyframe;
    bool animating = m_animating;
    bool reverse = m_reverse;
    m_keyframe = keyframe;
    m_time = 0;
    m_animating = true;
    if (time < 0) {
        time = -time;
        m_reverse = !m_reverse;
        int start = m_startKeyframe;
        m_startKeyframe = m_endKeyframe;
        m_endKeyframe = start;
    }
    update(time);
    m_animating = animating;
    m_reverse = reverse;
    m_startKeyframe = startKeyframe;
    m_endKeyframe = endKeyframe;
}

void AnimPlayerQuad::snapToTime(int time) {
    snapToTime(m_startKeyframe, time);
}

void AnimPlayerQuad::playAnimAcross(int mode, int from, int to) {
    m_animating = true;
    m_startKeyframe = from;
    m_endKeyframe = to;
    m_loop = mode & PLAY_LOOP;
    m_reverse = to <= from;
    bool keep;
    if (to > from) {
        keep = m_animating && !(mode & PLAY_RESTART) && m_keyframe >= from && m_keyframe < to;
    } else {
        keep = m_animating && !(mode & PLAY_RESTART) && m_keyframe >= to && m_keyframe < from;
    }
    if (!keep) {
        m_keyframe = from;
        m_time = 0;
        refreshMeshes();
    }
    m_animating = !(m_keyframe == m_endKeyframe && m_time == 0);
}

void AnimPlayerQuad::playAnimReverse(int mode) {
    if (!m_animating || (mode & PLAY_RESTART)) {
        m_keyframe = getLastKeyframeIndex();
        m_time = 0;
        m_animating = true;
        refreshMeshes();
    }
    m_startKeyframe = getLastKeyframeIndex();
    m_endKeyframe = 0;
    m_loop = mode & PLAY_LOOP;
    m_reverse = true;
    m_animating = !(m_keyframe == m_endKeyframe && m_time == 0);
}

void AnimPlayerQuad::playAnim(int mode) {
    if (!m_animating || (mode & PLAY_RESTART)) {
        m_keyframe = 0;
        m_time = 0;
        m_animating = true;
        refreshMeshes();
    }
    m_startKeyframe = 0;
    m_endKeyframe = getLastKeyframeIndex();
    m_loop = mode & PLAY_LOOP;
    m_reverse = false;
    m_animating = m_keyframe != m_endKeyframe;
}

void AnimPlayerQuad::snapToFrame(int keyframe) {
    m_keyframe = keyframe;
    m_time = 0;
    m_animating = false;
    refreshMeshes();
}

void AnimPlayerQuad::setNumKeyframes(int count) {
    m_keyframeTimes = midp::array<int>(count);
    for (int i = m_attribs.m_length; i != 0;) {
        i--;
        m_attribs[i].m_values = midp::array<int>(count);
    }
    m_startKeyframe = 0;
    m_endKeyframe = count - 1;
}

void AnimPlayerQuad::setNumAttributes(int count) {
    m_attribs = midp::array<QuadAttrib>(count);
}

int AnimPlayerQuad::getLastKeyframeIndex() {
    return m_keyframeTimes.m_length - 1;
}

void AnimPlayerQuad::setKeyframeTime(int keyframe, int time) {
    m_keyframeTimes[keyframe] = time;
}

void AnimPlayerQuad::setAttribValue(int attrib, int keyframe, int value) {
    m_attribs[attrib].m_values[keyframe] = value;
}
