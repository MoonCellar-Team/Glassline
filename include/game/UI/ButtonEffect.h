#pragma once

namespace UI {

class ButtonEffect {
public:
    ButtonEffect();
    void update(int dt);
    void play(int x, int y);
    bool isAnimating() const;

    bool m_animating;
};

}
