#pragma once

#include "game/Menu.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Image;
}

class MenuMain : public Menu {
public:
    enum Item {
        ITEM_ABOUT = 0x801,
        ITEM_HELP = 0x803,
        ITEM_LEADERBOARDS = 0x81d,
        ITEM_OPTIONS = 0x81e,
        ITEM_SETTINGS = 0x822,
        ITEM_EXTRAS = 0x823,
        ITEM_BADGES = 0x825,
        ITEM_SOUND = 0x8d7,
        ITEM_WALLPAPERS = 0x901,
        ITEM_LANGUAGE = 0x903,
        ITEM_MULTIPLAYER = 0x930,
        ITEM_INFO = 0x95c,
#ifdef MEDECOMP_PORT
        ITEM_PLAYER_LEVELS = 0x99e,
        ITEM_QUIT = 0x9bd,
        ITEM_LAUNCHER = 0x9c6,
        ITEM_SOLO_BAGS = 0x9cd,
#endif
    };

    MenuMain();
    virtual ~MenuMain();
    virtual bool pointerReleased(int x, int y, int localX, int localY, int width, int height, GameScreen* screen);
    virtual void render(midp::Graphics* g);
    virtual void activateSubMenu(int index, int item);

    bool pointerReleased(int x, int y, int width, int height);
    void pointerDragged(int x, int y, int width, int height);
    void pointerPressed(int x, int y, int width, int height);
    using Menu::pointerDragged;
    using Menu::pointerPressed;
    void update(int dt);
    void create();

    float m_bannerOffset;
    float m_bannerTarget;
    int m_bannerDelay;
    midp::ReferenceCountedPointer<midp::Image> m_banner;
};
