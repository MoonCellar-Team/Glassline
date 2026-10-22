#include "game/UI/AboutMenu.h"

#include "ea_online/SpywareManager.h"
#include "game/AppEngine.h"
#include "game/SceneMenu.h"
#include "game/TextManager.h"

static const int TEXT_TITLE = 0x801;
static const int TEXT_SUBTITLE = 0x95c;
static const int TEXT_SHARE = 0x911;

static const int FRAME_X = 0x6e;
static const int FRAME_Y = 0x122;
static const int FRAME_WIDTH = 0x324;
static const int FRAME_HEIGHT = 0xb4;
static const int PANEL_LIFT = 0x1c;
static const int ARROW_INSET = 0x24;
static const int SHARE_GAP = 10;
static const int SHARE_BOTTOM = 0x38;
static const int LABEL_GAP = 10;
#ifdef MEDECOMP_PORT
static const int OFFSCREEN = -3000;
#endif

static const int FONT_SHARE = 2;
static const int ANCHOR_SHARE = 0x14;

namespace UI {

void AboutMenu::setHidden(bool hidden) {
    m_hidden = hidden;
}

bool AboutMenu::pointerPressed(int x, int y, int id) {
    if (m_hidden) {
        return false;
    }
    TitledWindow::pointerPressed(x, y, id);
    if (m_panel.contains(x, y)) {
        m_panel.pointerPressed(m_panel.toRelativeX(x), y, id);
    }
    return true;
}

void AboutMenu::update(int dt) {
    if (m_hidden) {
        return;
    }
    TitledWindow::update(dt);
    m_panel.update(dt);
    m_share.setChecked(SpywareManager::getInstance()->isEnabled());
}

void AboutMenu::render(midp::Graphics* g, int originY, int originX) {
    if (m_hidden) {
        return;
    }
    TitledWindow::render(g, originY, originX);
    m_panel.render(g, originY, originX);
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int labelX = m_share.getX() - LABEL_GAP;
    int labelY = m_share.getY() + (m_share.getHeight() >> 1);
    text->drawString(g, TEXT_SHARE, FONT_SHARE, labelX, labelY, ANCHOR_SHARE);
    m_share.render(g, originY, originX);
    m_nextButton.render(g, originY, originX);
    m_prevButton.render(g, originY, originX);
}

bool AboutMenu::pointerReleased(int x, int y, int id) {
    if (m_hidden) {
        return false;
    }
    TitledWindow::pointerReleased(x, y, id);
    if (m_nextButton.contains(x, y)) {
        m_panel.next();
        m_nextButton.pointerReleased(x, y, id);
        return true;
    }
    if (m_prevButton.contains(x, y)) {
        m_panel.prev();
        m_prevButton.pointerReleased(x, y, id);
        return true;
    }
    if (m_panel.contains(x, y)) {
        m_panel.pointerReleased(x, y, id);
        return true;
    }
    if (m_share.contains(x, y)) {
        bool checked = m_share.getChecked();
        SceneMenu* scene = AppEngine::getCanvas()->getSceneMenu();
        scene->stateTransition(checked ? SceneMenu::STATE_DISABLE_SHARE_CONFIRM
                                       : SceneMenu::STATE_ENABLE_SHARE_CONFIRM);
        setHidden(true);
    }
    return true;
}

bool AboutMenu::pointerDragged(int x, int y, int id) {
    if (m_hidden) {
        return false;
    }
    TitledWindow::pointerDragged(x, y, id);
    if (m_panel.contains(x, y)) {
        m_panel.pointerDragged(m_panel.toRelativeX(x), y, id);
    } else if (m_panel.isDragging()) {
        m_panel.pointerReleased(x, y, id);
    }
    return true;
}

AboutMenu::AboutMenu()
    : TitledWindow(TEXT_TITLE, TEXT_SUBTITLE),
      m_hidden(false),
      m_panel(this) {
    m_background.setPosition(FRAME_X, FRAME_Y);
    m_background.setDimensions(FRAME_WIDTH, FRAME_HEIGHT);

    int panelX = (m_width - m_panel.getWidth()) >> 1;
    int panelY = m_background.getY() + ((m_background.getHeight() - m_panel.getHeight()) >> 1) - PANEL_LIFT;
    m_panel.setPosition(panelX, panelY);

    int prevY = m_panel.getY() + ((m_panel.getHeight() - m_prevButton.getHeight()) >> 1);
    m_prevButton.setPosition(m_background.getX() + ARROW_INSET, prevY);
    int nextX = (m_background.getX() + m_background.getWidth() - ARROW_INSET) - m_prevButton.getWidth();
    int nextY = m_panel.getY() + ((m_panel.getHeight() - m_nextButton.getHeight()) >> 1);
    m_nextButton.setPosition(nextX, nextY);

    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int textWidth = text->getStringWidth(TEXT_SHARE, FONT_SHARE);
    int boxWidth = m_share.getWidth();
    int shareX = ((textWidth + boxWidth + m_width + SHARE_GAP) >> 1) - boxWidth;
    int shareY = m_background.getY() + m_background.getHeight() - SHARE_BOTTOM;
    m_share.setPosition(shareX, shareY);
    m_share.setChecked(SpywareManager::getInstance()->isEnabled());
#ifdef MEDECOMP_PORT
    m_share.setPosition(OFFSCREEN, OFFSCREEN);
#endif
}

AboutMenu::~AboutMenu() {}

}
