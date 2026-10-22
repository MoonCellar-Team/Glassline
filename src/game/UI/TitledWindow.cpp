#include "game/UI/TitledWindow.h"

#include <stddef.h>

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"

static const int TEXT_BACK = 0x82f;
static const int NO_TEXT = -1;

static const int BACK_BUTTON_RIGHT = 69;
static const int BACK_BUTTON_BOTTOM = 36;

static const int FONT_TITLE = 16;
static const int FONT_SUBTITLE = 17;
static const int TITLE_OFFSET_X = 4;
static const int TITLE_OFFSET_Y = -7;
static const int ANCHOR_TITLE = 33;
static const int ANCHOR_SUBTITLE = 65;

namespace UI {

void TitledWindow::update(int dt) {
    if (m_backButton.isActivated()) {
        m_backButton.reset();
        close(RESULT_OK);
    }
}

bool TitledWindow::pointerDragged(int x, int y, int id) {
    if (m_backButton.isPressed() && !m_backButton.contains(x, y)) {
        m_backButton.unpress();
    }
    return false;
}

bool TitledWindow::pointerPressed(int x, int y, int id) {
    if (m_backButton.contains(x, y)) {
        m_backButton.pointerPressed(x, y, id);
        return true;
    }
    return false;
}

TitledWindow::~TitledWindow() {}

void TitledWindow::render(midp::Graphics* g, int originY, int originX) {
    if (m_showBackground) {
        m_background.render(g, originY, originX);
    }
    m_backButton.render(g, originY, originX);
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int lineHeight = text->getLineHeight(FONT_TITLE);
    int x = m_background.getX() + TITLE_OFFSET_X;
    int y = m_background.getY() + TITLE_OFFSET_Y;
    text->drawString(g, m_title, FONT_TITLE, x, y, ANCHOR_TITLE);
    if (m_subtitle != NULL) {
        text->drawString(g, m_subtitle, FONT_SUBTITLE, x, y - lineHeight, ANCHOR_SUBTITLE);
    }
}

bool TitledWindow::pointerReleased(int x, int y, int id) {
    if (m_backButton.contains(x, y)) {
        m_backButton.pointerReleased(x, y, id);
        return true;
    }
    if (m_backButton.isPressed()) {
        m_backButton.unpress();
    }
    return false;
}

void TitledWindow::setTitles(int title, int subtitle) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_title = text->getString(title)->toUpperCase();
    if (subtitle != NO_TEXT) {
        m_subtitle = text->getString(subtitle)->toUpperCase();
    }
}

TitledWindow::TitledWindow(int title, int subtitle)
    : m_title(NULL),
      m_subtitle(NULL),
      m_background(0, 0, 0, 0),
      m_backButton(TEXT_BACK),
      m_showBackground(true) {
    setTitles(title, subtitle);
    m_backButton.setPosition(m_width - BACK_BUTTON_RIGHT - m_backButton.getWidth(),
                             m_height - BACK_BUTTON_BOTTOM - m_backButton.getHeight());
}

void TitledWindow::setShowBackground(bool show) {
    m_showBackground = show;
}

}
