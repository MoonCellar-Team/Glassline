#pragma once

#include "game/GameUI/Button.h"
#include "game/GameUI/SolidRect.h"
#include "game/GameUI/WindowBacking.h"
#include "game/WrappedString.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class GameScreen;
class SceneGame;
namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
class String;
}

namespace GameUI {

class ScrollingText : public midp::ReferenceCounted {
public:
    ScrollingText(SceneGame* scene, GameScreen* screen, int titleId, int headerId, int textId);
    virtual ~ScrollingText();

    void update(int dt);
    void render2D(midp::Graphics* g);
    void render3D(m3g::Graphics3D* g3d);
    void pointerPressed(int x, int y, int id);
    void pointerDragged(int pressX, int pressY, int x, int y, int id);
    int pointerReleased(int pressX, int pressY, int x, int y, int id, GameScreen* screen);
    void moveScroller(int delta);

    SceneGame* m_scene;
    GameScreen* m_screen;
    midp::ReferenceCountedPointer<midp::String> m_title;
    midp::ReferenceCountedPointer<midp::String> m_header;
    int m_titleY;
    int m_headerY;
    WindowBacking m_backing;
    WrappedString m_text;
    SolidRect m_thumb;
    SolidRect m_track;
    Button m_button;
    int m_viewHeight;
    int m_textHeight;
    int m_prevScroll;
    int m_scroll;
    int m_pointerId;
    int m_pointerY;
    int m_dragScroll;
    float m_velocity;
};

}
