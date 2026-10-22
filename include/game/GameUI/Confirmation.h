#pragma once

#include "game/GameUI/Button.h"
#include "game/GameUI/WindowBacking.h"
#include "game/WrappedString.h"
#include "runtime/midp/ReferenceCounted.h"

class GameScreen;
namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
}

namespace GameUI {

class Confirmation : public midp::ReferenceCounted {
public:
    enum Result {
        RESULT_NONE = 0,
        RESULT_YES = 1,
        RESULT_NO = 2,
    };

    Confirmation(int textId, int width, int height, int yesId, int noId);
    ~Confirmation();
    void render2D(midp::Graphics* g);
    void render3D(m3g::Graphics3D* g3d);
    void pointerPressed(int x, int y);
    void pointerDragged(int pressX, int pressY, int x, int y);
    int pointerReleased(int pressX, int pressY, int x, int y, GameScreen* screen);
    void setupWindow(int textId, int width, int height);

    WindowBacking m_backing;
    WrappedString m_text;
    Button m_yes;
    Button m_no;
};

}
