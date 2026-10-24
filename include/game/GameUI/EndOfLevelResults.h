#pragma once

#include "game/GameUI/Button.h"
#include "game/GameUI/SingleQuad.h"
#include "game/GameUI/SolidRect.h"

class GameScreen;
class SceneGame;
namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
}

namespace GameUI {

class EndOfLevelResults : public midp::ReferenceCounted {
public:
    EndOfLevelResults(SceneGame* scene, GameScreen* screen);
    virtual ~EndOfLevelResults();

    void render2D(midp::Graphics* g);
    void render3D(m3g::Graphics3D* g3d);
    void pointerPressed(int x, int y);
    void pointerDragged(int pressX, int pressY, int x, int y);
    int pointerReleased(int pressX, int pressY, int x, int y, GameScreen* screen);
#ifdef MEDECOMP_PORT
    Button* netButton(int index);
#endif

    SceneGame* m_scene;
    GameScreen* m_screen;
    SolidRect m_backdrop;
    SingleQuad m_quad;
    Button m_button;
};

}
