#pragma once

#include "game/GameUI/Button.h"

class GameScreen;
namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
}

namespace GameUI {

class BottomButtonSet {
public:
    enum Result {
        RESULT_NONE = 0,
        RESULT_BUTTON = 1,
        RESULT_SECOND_BUTTON = 2,
    };

    BottomButtonSet(int labelId, GameScreen* screen);
    ~BottomButtonSet();
    int pointerReleased(int pressX, int pressY, int x, int y);
    void pointerDragged(int pressX, int pressY, int x, int y);
    void pointerPressed(int x, int y);
    void render2D(midp::Graphics* g);
    void render3D(m3g::Graphics3D* g3d);

    GameScreen* m_screen;
    Button m_button;
    Button m_secondButton;
};

}
