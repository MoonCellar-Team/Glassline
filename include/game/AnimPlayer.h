#pragma once

namespace midp {
class Graphics;
}

class AnimPlayer {
public:
    static const int START_LOOP = 4;
    static const int START_RESTART = 16;

    static const int FLAG_VALID = 1;
    static const int FLAG_ANIMATING = 2;
    static const int FLAG_LOOP = 4;
    static const int FLAG_REVERSE = 8;

    AnimPlayer();
    void startAnim(int anim, int flags);
    void prevFrame();
    void nextFrame();
    bool updateAnim(int dt);
    void drawAnim(midp::Graphics* g, int x, int y);

    int m_anim;
    int m_frame;
    int m_frameTime;
    int m_flags;
};
