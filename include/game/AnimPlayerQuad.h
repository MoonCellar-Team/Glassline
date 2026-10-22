#pragma once

#include "game/QuadAttrib.h"
#include "runtime/midp/array.h"

class QuadManager;
class QuadMesh;

class AnimPlayerQuad {
public:
    static const int PLAY_LOOP = 1;
    static const int PLAY_RESTART = 2;

    AnimPlayerQuad(QuadManager* manager, int id);

    bool isAnimating();
    void stopAnim();
    bool isAnimatingBackward();
    bool isAnimatingForward();
    int getCurrentKeyframe();
    void playAnimTo(int keyframe);
    void nextFrame();
    void previousFrame();
    void setupAttribute(int attrib, QuadMesh* mesh, int meshAttrib);
    void refreshMeshes();
    void update(int dt);
    void snapToTime(int keyframe, int time);
    void snapToTime(int time);
    void playAnimAcross(int mode, int from, int to);
    void playAnimReverse(int mode);
    void playAnim(int mode);
    void snapToFrame(int keyframe);
    void setNumKeyframes(int count);
    void setNumAttributes(int count);
    int getLastKeyframeIndex();
    void setKeyframeTime(int keyframe, int time);
    void setAttribValue(int attrib, int keyframe, int value);

    QuadManager* m_manager;
    int m_id;
    midp::array<QuadAttrib> m_attribs;
    midp::array<int> m_keyframeTimes;
    int m_keyframe;
    int m_time;
    bool m_animating;
    int m_startKeyframe;
    int m_endKeyframe;
    bool m_loop;
    bool m_reverse;
};
