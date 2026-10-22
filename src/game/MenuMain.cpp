#include "game/MenuMain.h"

#include <math.h>
#include <stddef.h>

#include "game/AppEngine.h"
#include "game/MenuMainSubMenu.h"
#include "game/QuadManager.h"
#include "m3g/Graphics3D.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/RuntimeIPhone.h"

static const int MESH_MORE_GAMES = 94;
static const int SCOPE_MENU = 0x200;

static const float BANNER_HIDDEN = -163.0f;
static const float BANNER_SPEED = 0.22f;
static const float BANNER_EPSILON = 0.01f;

bool MenuMain::pointerReleased(int x, int y, int width, int height) {
    return pointerReleased(x, y, x, y, width, height, NULL);
}

void MenuMain::pointerDragged(int x, int y, int width, int height) {
    Menu::pointerDragged(x, y, x, y, width, height, NULL);
}

void MenuMain::pointerPressed(int x, int y, int width, int height) {
    Menu::pointerPressed(x, y, x, y, width, height, NULL);
}

MenuMain::~MenuMain() {}

bool MenuMain::pointerReleased(int x, int y, int localX, int localY, int width, int height, GameScreen* screen) {
    return Menu::pointerReleased(x, y, localX, localY, width, height, screen);
}

void MenuMain::render(midp::Graphics* g) {
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    int width = canvas->getWidth();
    int height = canvas->getHeight();
    m3g::Graphics3D* g3d = AppEngine::getCanvas()->getGraphics3D();
    g3d->bindTarget(g);
    Menu::render3D(g, g3d, width, height);
    g3d->releaseTarget();
    quads.render(g, SCOPE_MENU);
    Menu::render2D(g, width, height);
}

void MenuMain::activateSubMenu(int index, int item) {
    bool activated = false;
    m_selectionIndex = index;
    for (int i = 0; i != m_subMenus.m_length; i++) {
        if (i == m_selectionIndex && m_subMenus[i].getLength() > 0 && !m_subMenus[i].isActive()) {
            m_subMenus[i].transitionToActive();
            activated = true;
        } else {
            m_subMenus[i].transitionToIdle();
        }
    }
    if (m_bannerTarget == 0.0f && activated) {
        m_bannerTarget = BANNER_HIDDEN;
    } else if (m_bannerTarget == BANNER_HIDDEN && !activated) {
        m_bannerTarget = 0.0f;
    }
    MenuMainSubMenu& selected = m_subMenus[m_selectionIndex];
    if (selected.getLength() == 0) {
        selected.pointerOn(0, 0);
    } else if (item >= 0) {
        selected.setSelectedItem(item);
    }
}

MenuMain::MenuMain() : m_bannerOffset(0.0f), m_bannerTarget(0.0f), m_bannerDelay(0), m_banner(NULL) {}

void MenuMain::update(int dt) {
    Menu::update(dt);
    if (m_bannerDelay > 0) {
        m_bannerDelay -= dt;
        return;
    }
    float diff = m_bannerOffset - m_bannerTarget;
    if (fabsf(diff) <= BANNER_EPSILON) {
        return;
    }
    if (diff >= 0.0f) {
        m_bannerOffset += dt * -BANNER_SPEED;
        if (m_bannerOffset < m_bannerTarget) {
            m_bannerOffset = m_bannerTarget;
        }
    } else {
        m_bannerOffset += dt * BANNER_SPEED;
        if (m_bannerOffset > m_bannerTarget) {
            m_bannerOffset = m_bannerTarget;
        }
    }
}

void MenuMain::create() {
    AppEngine* canvas = AppEngine::getCanvas();
    int height = canvas->getHeight();
    m_subMenus = midp::array<MenuMainSubMenu>(4);
    m_labelWidth = 160;
    m_subMenus[0].init(height, 59, 90, 63, 12);
    m_subMenus[0].setScollingUVs(70, 71, 72, 73);
    m_subMenus[1].init(height, 60, 91, 64, 13);
    m_subMenus[1].setScollingUVs(74, 75, 76, 77);
    m_subMenus[1].create(3, ITEM_OPTIONS);
    m_subMenus[1].append(ITEM_SETTINGS);
    m_subMenus[1].append(ITEM_SOUND);
    if (canvas->getTextManager()->getLanguageCount() > 1) {
        m_subMenus[1].append(ITEM_LANGUAGE);
    }
    m_subMenus[2].init(height, 61, 92, 65, 14);
    m_subMenus[2].setScollingUVs(78, 79, 80, 81);
#ifdef MEDECOMP_PORT
    m_subMenus[2].create(5, ITEM_EXTRAS);
#else
    m_subMenus[2].create(4, ITEM_EXTRAS);
#endif
    static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->isOSMediaPickerSupported();
    m_subMenus[2].append(ITEM_BADGES);
    m_subMenus[2].append(ITEM_WALLPAPERS);
    m_subMenus[2].append(ITEM_LEADERBOARDS);
#ifdef MEDECOMP_PORT
    m_subMenus[2].append(ITEM_PLAYER_LEVELS);
    m_subMenus[2].append(ITEM_MULTIPLAYER);
#endif
    m_subMenus[3].init(height, 62, 93, 66, 15);
    m_subMenus[3].setScollingUVs(82, 83, 84, 85);
#ifdef MEDECOMP_PORT
    m_subMenus[3].create(4, ITEM_INFO);
#else
    m_subMenus[3].create(2, ITEM_INFO);
#endif
    m_subMenus[3].append(ITEM_HELP);
    m_subMenus[3].append(ITEM_ABOUT);
#ifdef MEDECOMP_PORT
    m_subMenus[3].append(ITEM_LAUNCHER);
    m_subMenus[3].append(ITEM_QUIT);
#endif
    canvas->getQuadManager().setMeshVisible(MESH_MORE_GAMES, false);
}
