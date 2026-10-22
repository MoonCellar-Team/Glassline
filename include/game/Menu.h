#pragma once

#include "runtime/midp/array.h"

class GameScreen;
class MenuMainSubMenu;
namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
}

class Menu {
public:
    Menu();
    virtual ~Menu();
    virtual void render3D(midp::Graphics* g, m3g::Graphics3D* g3d, int width, int height);
    virtual void render2D(midp::Graphics* g, int width, int height);
    virtual void pointerPressed(int x, int y, int localX, int localY, int width, int height, GameScreen* screen);
    virtual void pointerDragged(int x, int y, int localX, int localY, int width, int height, GameScreen* screen);
    virtual bool pointerReleased(int x, int y, int localX, int localY, int width, int height, GameScreen* screen);

    int getSelectionIndex();
    MenuMainSubMenu* getSelectedSubMenu();
    bool isItemSelected();
    MenuMainSubMenu* getSubMenu(int index);
    bool isAnySubMenuAnimating();
    void reset();
    void setSubMenuOpen(int index, int item);
    void activateSubMenu(int index, GameScreen* screen, int item);
    void update(int dt);
    void wrapStrings(bool upperCase);
    void setSelectionIndex(int index);
    bool isCreated() const;

    int m_labelWidth;
    int m_pressedIndex;
    bool m_originHidden;
    midp::array<MenuMainSubMenu> m_subMenus;
    int m_selectionIndex;
};
