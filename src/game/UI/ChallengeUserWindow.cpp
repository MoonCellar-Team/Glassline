#include "game/UI/ChallengeUserWindow.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "game/UI/NetworkWaitEffect.h"

static const int TEXT_BACK = 0x82f;
static const int TEXT_EXTRA = 0x94b;
static const int TEXT_TITLE = 0x92d;
static const int TEXT_SUBTITLE = 0x95e;
static const int FONT_TITLE = 0x10;
static const int FONT_TITLE_HEIGHT = 0x11;
static const int FONT_BODY = 2;
static const int ANCHOR_TITLE = 9;
static const int ANCHOR_SUBTITLE = 10;

namespace UI {

bool ChallengeUserWindow::pointerDragged(int x, int y, int id) {
    m_dragged = true;
    if (!m_backButton.contains(x, y) && m_backButton.isPressed()) {
        m_backButton.unpress();
    }
    if (m_users.contains(x, y)) {
        m_users.pointerDragged(m_users.toRelativeX(x), m_users.toRelativeY(y), id);
    }
    if (!m_extraButton.contains(x, y) && m_extraButton.isPressed()) {
        m_extraButton.unpress();
    }
    return true;
}

bool ChallengeUserWindow::pointerPressed(int x, int y, int id) {
    if (m_backButton.contains(x, y)) {
        m_backButton.pointerPressed(x, y, id);
    }
    if (m_users.contains(x, y)) {
        m_users.pointerPressed(m_users.toRelativeX(x), m_users.toRelativeY(y), id);
    }
    if (m_extraButton.contains(x, y)) {
        m_extraButton.pointerPressed(x, y, id);
    }
    return true;
}

void ChallengeUserWindow::update(int dt) {
    m_users.update(dt);
}

void ChallengeUserWindow::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_backButton.render(g, originY, originX);
    m_frame.render(g, originY, originX);
    int titleY = text->getLineHeight(FONT_TITLE_HEIGHT) + 7;
    text->drawString(g, m_unk1C, FONT_TITLE, 5, titleY, ANCHOR_TITLE);
    int subtitleY = m_frame.getY() + 5;
    text->drawString(g, TEXT_SUBTITLE, FONT_BODY, m_width >> 1, subtitleY, ANCHOR_SUBTITLE);
    m_users.render(g, originY, originX);
}

bool ChallengeUserWindow::pointerReleased(int x, int y, int id) {
    if (m_backButton.contains(x, y)) {
        m_backButton.pointerReleased(x, y, id);
        close(RESULT_CANCEL);
        AppEngine* engine = AppEngine::getCanvas();
        if (engine->getWindowStore().getNetworkWaitEffect()->isAnimating()) {
            AppEngine::getCanvas()->getWindowStore().getNetworkWaitEffect()->stop();
        }
    } else if (m_backButton.isPressed()) {
        m_backButton.unpress();
    }
    if (m_users.contains(x, y) && !m_dragged) {
        int relX = m_users.toRelativeX(x);
        int relY = m_users.toRelativeY(y);
        if (m_users.pointerReleased(relX, relY, id)) {
            close(RESULT_OK);
        }
    }
    m_dragged = false;
    return true;
}

ChallengeUserWindow::ChallengeUserWindow()
    : m_backButton(TEXT_BACK),
      m_extraButton(TEXT_EXTRA),
      m_frame(0x32, 0x46, m_width - 100, m_height - 0x8c),
      m_users(*this, 0x32, 0x50, m_width - 100, m_height - 0x96),
      m_dragged(false) {
    m_unk1C = TEXT_TITLE;
    int backW = m_backButton.getWidth();
    int backH = m_backButton.getHeight();
    m_backButton.setPosition((m_width - 5) - backW, (m_height - 5) - backH);
}

ChallengeUserWindow::~ChallengeUserWindow() {}

}
