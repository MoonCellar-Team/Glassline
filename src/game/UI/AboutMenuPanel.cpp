#include "game/UI/AboutMenuPanel.h"

#include "game/AppEngine.h"
#include "game/SceneMenu.h"
#include "game/UI/AboutMenu.h"
#include "game/UI/AboutMenuItem.h"

static const int PANEL_WIDTH = 0x212;
static const int PANEL_HEIGHT = 0x50;
static const int NOTCH_WIDTH = 0x230;
static const int RENDER_EXTRA = 2;
static const float FIRST_NOTCH_CENTER = 280.0f;

static const int TEXT_ABOUT = 0x801;
static const int TEXT_EULA = 0x907;
static const int TEXT_PRIVACY = 0x908;
static const int TEXT_TERMS = 0x909;

static const SceneMenu::MenuState STATE_ABOUT = static_cast<SceneMenu::MenuState>(14);
static const SceneMenu::MenuState STATE_EULA = static_cast<SceneMenu::MenuState>(23);
static const SceneMenu::MenuState STATE_PRIVACY = static_cast<SceneMenu::MenuState>(24);
static const SceneMenu::MenuState STATE_TERMS = static_cast<SceneMenu::MenuState>(25);

namespace UI {

bool AboutMenuPanel::pointerReleased(int x, int y, int id) {
    if (!m_dragging && m_selected != NULL) {
        int textId = static_cast<AboutMenuItem*>(m_selected)->getStringId();
        SceneMenu* scene = AppEngine::getCanvas()->getSceneMenu();
        if (textId > 0x906) {
            if (textId == TEXT_EULA) {
                scene->stateTransition(STATE_EULA);
            } else if (textId == TEXT_PRIVACY) {
                scene->stateTransition(STATE_PRIVACY);
            } else if (textId == TEXT_TERMS) {
                scene->stateTransition(STATE_TERMS);
            }
        } else if (textId == TEXT_ABOUT) {
            scene->stateTransition(STATE_ABOUT);
        }
        static_cast<AboutMenu*>(m_parent)->setHidden(true);
    }
    NotchedSlider::pointerReleased(x, y, id);
    return true;
}

AboutMenuPanel::AboutMenuPanel(AboutMenu* menu) {
    m_parent = menu;
    setWidth(PANEL_WIDTH);
    setHeight(PANEL_HEIGHT);
    addItem(new AboutMenuItem(TEXT_ABOUT));
    addItem(new AboutMenuItem(TEXT_EULA));
    addItem(new AboutMenuItem(TEXT_PRIVACY));
    addItem(new AboutMenuItem(TEXT_TERMS));
    setNotchWidth(NOTCH_WIDTH);
    setRenderExtra(RENDER_EXTRA);
    m_halfNotchWidth = FIRST_NOTCH_CENTER;
}

AboutMenuPanel::~AboutMenuPanel() {}

}
