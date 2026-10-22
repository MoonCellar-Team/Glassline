#pragma once

namespace UI {

class NetworkWaitEffect {
public:
    NetworkWaitEffect();
    bool isAnimating() const;
    void hardStop();
    void update(int dt);
    void play(int x, int y);
    void stop();

    int m_x;
    int m_y;
    bool m_animating;
};

}
