#include "game/Menu.h"

#include <stddef.h>

#include "ea_online/ea/EASpywareManager.h"
#include "game/AppEngine.h"
#include "game/GameScreen.h"
#include "game/MenuMainSubMenu.h"
#include "game/QuadManager.h"
#include "game/UI/ButtonEffect.h"
#include "game/UI/WindowStore.h"
#include "runtime/midp/Math.h"

static const int LAST_SUBMENU = 3;
static const int SUBMENU_INFO = 3;
static const int FONT_ITEMS = 7;

int Menu::getSelectionIndex() {
    return m_selectionIndex;
}

MenuMainSubMenu* Menu::getSelectedSubMenu() {
    if (m_selectionIndex < 0) {
        return NULL;
    }
    return &m_subMenus[m_selectionIndex];
}

bool Menu::isItemSelected() {
    if (m_selectionIndex == -1) {
        return false;
    }
    return m_subMenus[m_selectionIndex].isSelected();
}

MenuMainSubMenu* Menu::getSubMenu(int index) {
    return &m_subMenus[index];
}

bool Menu::isAnySubMenuAnimating() {
    int count = m_subMenus.m_length;
    for (int i = 0; i < count; i++) {
        if (m_subMenus[i].isAnimating()) {
            return true;
        }
    }
    return false;
}

Menu::~Menu() {}

void Menu::render3D(midp::Graphics* g, m3g::Graphics3D* g3d, int width, int height) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    for (int i = 0; i != m_subMenus.m_length; i++) {
        m_subMenus[i].refreshAnim(quads);
    }
    quads.render(g3d);
    for (int i = 0; i != m_subMenus.m_length; i++) {
        m_subMenus[i].render3D(g, g3d, width, height);
    }
}

void Menu::render2D(midp::Graphics* g, int width, int height) {
    for (int i = 0; i != m_subMenus.m_length; i++) {
        m_subMenus[i].render2D(g, m_labelWidth, width, height);
    }
}

void Menu::pointerPressed(int x, int y, int localX, int localY, int width, int height, GameScreen* screen) {
    AppEngine* canvas = AppEngine::getCanvas();
    int index = localX / (width >> 2);
    QuadManager& quads = canvas->getQuadManager();
    int selected = m_selectionIndex;
    bool visible = quads.getMeshVisible(m_subMenus[index].getButtonMeshId());
    quads.setMeshVisible(m_subMenus[index].getButtonMeshId(), true);
    bool inside = quads.isPointWithinMesh(m_subMenus[index].getButtonMeshId(), localX, localY);
    quads.setMeshVisible(m_subMenus[index].getButtonMeshId(), visible);
    if (inside) {
        m_pressedIndex = index;
    }
    if (selected == index) {
        m_subMenus[index].pointerOn(localX, localY);
    }
}

void Menu::pointerDragged(int x, int y, int localX, int localY, int width, int height, GameScreen* screen) {
    int selected = m_selectionIndex;
    int index = midp::Math::min(LAST_SUBMENU, localX / (width >> 2));
    if (selected == index) {
        m_subMenus[index].pointerOn(localX, localY);
    } else {
        m_subMenus[index].pointerOff(localX, localY);
    }
}

bool Menu::pointerReleased(int x, int y, int localX, int localY, int width, int height, GameScreen* screen) {
    if (isAnySubMenuAnimating()) {
        return false;
    }
    AppEngine* canvas = AppEngine::getCanvas();
    int index = localX / (width >> 2);
    QuadManager& quads = canvas->getQuadManager();
    int selected = m_selectionIndex;
    index = midp::Math::min(LAST_SUBMENU, index);
    int mesh = m_subMenus[index].getButtonMeshId();
    bool visible = quads.getMeshVisible(mesh);
    quads.setMeshVisible(mesh, true);
    bool inside = quads.isPointWithinMesh(mesh, localX, localY);
    quads.setMeshVisible(mesh, visible);
    if (inside && m_pressedIndex == index) {
        m_pressedIndex = -1;
        activateSubMenu(index, screen, -1);
        int effectX = quads.getMeshX(mesh);
        int effectY = quads.getMeshY(mesh);
        if (screen != NULL) {
            screen->calcGlobalCoordinates(effectX, effectY, effectX, effectY);
        }
        AppEngine::getCanvas()->getWindowStore().getButtonEffect()->play(effectX, effectY);
        return true;
    }
    if (selected == index) {
        m_subMenus[index].pointerReleased(localX, localY, screen);
    }
    m_pressedIndex = -1;
    return false;
}

Menu::Menu() : m_labelWidth(0), m_pressedIndex(-1), m_originHidden(false), m_selectionIndex(-1) {}

void Menu::reset() {
    m_selectionIndex = -1;
    for (int i = 0; i != m_subMenus.m_length; i++) {
        m_subMenus[i].stateTransition(MenuMainSubMenu::ANIM_IDLE);
    }
}

void Menu::setSubMenuOpen(int index, int item) {
    m_selectionIndex = index;
    m_subMenus[index].setOpen(item);
}

void Menu::activateSubMenu(int index, GameScreen* screen, int item) {
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
    MenuMainSubMenu& selected = m_subMenus[m_selectionIndex];
    if (selected.getLength() == 0) {
        selected.pointerOn(0, 0);
    } else if (item >= 0) {
        selected.setSelectedItem(item);
    }
    if (index == SUBMENU_INFO && activated) {
        ea::EASpywareManager::getInstance()->hideOrigin();
        m_originHidden = true;
    } else if (m_originHidden && screen == NULL) {
        ea::EASpywareManager::getInstance()->showOrigin(1);
        m_originHidden = false;
    }
}

void Menu::update(int dt) {
    for (int i = 0; i != m_subMenus.m_length; i++) {
        m_subMenus[i].update(dt);
    }
}

void Menu::wrapStrings(bool upperCase) {
    for (int i = 0; i != m_subMenus.m_length; i++) {
        m_subMenus[i].wrapStrings(FONT_ITEMS, m_labelWidth, upperCase);
    }
}

void Menu::setSelectionIndex(int index) {
    m_selectionIndex = index;
}

bool Menu::isCreated() const {
    return m_subMenus != NULL;
}
