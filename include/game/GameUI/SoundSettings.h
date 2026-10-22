#pragma once

#include "game/GameUI/BottomButtonSet.h"
#include "game/GameUI/SoundSlider.h"
#include "game/GameUI/WindowBackingTitled.h"
#include "runtime/midp/ReferenceCounted.h"

class GameScreen;
class SceneGame;
namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
}

namespace GameUI {

class SoundSettings : public midp::ReferenceCounted {
public:
    SoundSettings(SceneGame* scene, GameScreen* screen);
    virtual ~SoundSettings();

    void render2D(midp::Graphics* g);
    void render3D(m3g::Graphics3D* g3d);
    void pointerPressed(int x, int y, int id);
    void pointerDragged(int pressX, int pressY, int x, int y, int id);
    int pointerReleased(int pressX, int pressY, int x, int y, int id);

    bool canModifyMusicSlider();
    bool canModifyEffectSlider();

    SceneGame* m_scene;
    GameScreen* m_screen;
    WindowBackingTitled m_backing;
    BottomButtonSet m_buttons;
    SoundSlider m_music;
    SoundSlider m_effects;
};

}
