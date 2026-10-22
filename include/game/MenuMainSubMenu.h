#pragma once

#include "game/InterpolationTimed.h"
#include "game/MenuStringChoice.h"
#include "game/SignalFilter.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class GameScreen;
class QuadManager;
namespace m3g {
class Graphics3D;
class Texture2D;
}
namespace midp {
class Graphics;
}

class MenuMainSubMenu : public MenuStringChoice {
public:
    enum AnimState {
        ANIM_IDLE = 0,
        ANIM_OPENING = 1,
        ANIM_ACTIVE = 2,
        ANIM_CLOSING = 3,
    };

    MenuMainSubMenu();
    MenuMainSubMenu(const MenuMainSubMenu& other);
    virtual ~MenuMainSubMenu();

    bool isSelected();
    void init(int height, int ribbonGroup, int buttonMesh, int activeGroup, int anim);
    bool isAnimating();
    void pointerReleased(int x, int y, GameScreen* screen);
    void animateToSelection(int index, bool immediate);
    void pointerOff(int x, int y);
    void pointerOn(int x, int y);
    void refreshAnim(QuadManager& quads);
    void renderActiveRibbonLabels(midp::Graphics* g, int labelWidth, int width, int height);
    void render2D(midp::Graphics* g, int labelWidth, int width, int height);
    void render3D(midp::Graphics* g, m3g::Graphics3D* g3d, int width, int height);
    void setScollingUVs(int tex0, int tex1, int tex2, int tex3);
    void stateTransition(AnimState state);
    void setOpen(int index);
    void transitionToActive();
    void transitionToIdle();
    void update(int dt);
    int getButtonMeshId();
    bool isActive() const;
    int getSelectBarIndex();

    int m_screenHeight;
    bool m_selected;
    int m_animState;
    int m_unk64;
    int m_ribbonGroup;
    int m_buttonMesh;
    int m_activeGroup;
    int m_anim;
    SignalFilter m_sway;
    int m_swayTimer;
    int m_swayDir;
    float m_scroll0;
    float m_scroll1;
    midp::ReferenceCountedPointer<m3g::Texture2D> m_stripTex0;
    midp::ReferenceCountedPointer<m3g::Texture2D> m_stripTex1;
    midp::ReferenceCountedPointer<m3g::Texture2D> m_stripTex2;
    midp::ReferenceCountedPointer<m3g::Texture2D> m_stripTex3;
    int m_highlightIndex;
    int m_queuedIndex;
    InterpolationFloatTimed m_barAnim;
};
