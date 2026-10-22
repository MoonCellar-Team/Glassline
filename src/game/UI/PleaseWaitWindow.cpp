#include "game/UI/PleaseWaitWindow.h"

#include "game/AppEngine.h"
#include "game/UI/NetworkWaitEffect.h"
#include "game/UI/WindowStore.h"

static const int TEXT_PLEASE_WAIT = 0x92a;
static const int TEXT_OK = 0x82e;
static const int TEXT_RETRY = 0x94b;
static const int TEXT_BACK = 0x82f;

static const int FONT_MESSAGE = 2;
static const int FRAME_X = 0x2e;
static const int FRAME_WIDTH_CUT = 0x5c;
static const int TEXT_WIDTH_CUT = 0x84;
static const int FRAME_EXTRA_WAIT = 0x5a;
static const int FRAME_EXTRA_RESULT = 0x28;
static const int TEXT_TOP = 0x14;
static const int ANCHOR_TEXT = 10;
static const int BUTTON_MARGIN = 5;
static const int SPINNER_BELOW = 0x35;
static const int OFFSCREEN = -3000;

namespace UI {

bool PleaseWaitWindow::pointerPressed(int x, int y, int id) {
    if (m_okButton.contains(x, y)) {
        m_okButton.pointerPressed(x, y, id);
        return true;
    }
    if (m_backButton.contains(x, y)) {
        m_backButton.pointerPressed(x, y, id);
        return true;
    }
    if (m_retryButton.contains(x, y)) {
        m_retryButton.pointerPressed(x, y, id);
        return true;
    }
    return false;
}

bool PleaseWaitWindow::pointerDragged(int x, int y, int id) {
    if (m_okButton.isPressed() && !m_okButton.contains(x, y)) {
        m_okButton.unpress();
    }
    if (m_backButton.isPressed() && !m_backButton.contains(x, y)) {
        m_backButton.unpress();
    }
    if (m_retryButton.isPressed() && !m_retryButton.contains(x, y)) {
        m_retryButton.unpress();
    }
    return false;
}

void PleaseWaitWindow::render(midp::Graphics* g, int originY, int originX) {
    m_frame.render(g, originY, originX);
    m_text.draw(g, m_width >> 1, m_frame.getY() + TEXT_TOP, ANCHOR_TEXT);
    m_okButton.render(g, originY, originX);
    m_backButton.render(g, originY, originX);
    m_retryButton.render(g, originY, originX);
}

bool PleaseWaitWindow::pointerReleased(int x, int y, int id) {
    if (m_okButton.contains(x, y)) {
        m_okButton.pointerReleased(x, y, id);
        close(static_cast<WindowResult>(BUTTON_OK));
        return true;
    }
    if (m_okButton.isPressed()) {
        m_okButton.unpress();
    }
    if (m_backButton.contains(x, y)) {
        m_backButton.pointerReleased(x, y, id);
        close(static_cast<WindowResult>(BUTTON_BACK));
        return true;
    }
    if (m_backButton.isPressed()) {
        m_backButton.unpress();
    }
    if (m_retryButton.contains(x, y)) {
        m_retryButton.pointerReleased(x, y, id);
        close(static_cast<WindowResult>(BUTTON_RETRY));
        return true;
    }
    if (m_retryButton.isPressed()) {
        m_retryButton.unpress();
    }
    return false;
}

void PleaseWaitWindow::Failed(int textId) {
    m_text.wrapString(textId, FONT_MESSAGE, m_width - TEXT_WIDTH_CUT);
    int textHeight = m_text.getWrappedTextHeight();
    m_frame.setY((m_height - (textHeight + FRAME_EXTRA_RESULT)) >> 1);
    m_frame.setHeight(textHeight + FRAME_EXTRA_RESULT);
    int backX = (m_width - BUTTON_MARGIN) - m_backButton.getWidth();
    int backY = (m_height - BUTTON_MARGIN) - m_backButton.getHeight();
    m_backButton.setPosition(backX, backY);
    m_retryButton.setPosition((backX - BUTTON_MARGIN) - m_retryButton.getWidth(), backY);
}

void PleaseWaitWindow::Succeeded(int textId) {
    m_text.wrapString(textId, FONT_MESSAGE, m_width - TEXT_WIDTH_CUT);
    int textHeight = m_text.getWrappedTextHeight();
    m_frame.setY((m_height - (textHeight + FRAME_EXTRA_RESULT)) >> 1);
    m_frame.setHeight(textHeight + FRAME_EXTRA_RESULT);
    int okX = (m_width - BUTTON_MARGIN) - m_okButton.getWidth();
    int okY = (m_height - BUTTON_MARGIN) - m_okButton.getHeight();
    m_okButton.setPosition(okX, okY);
}

PleaseWaitWindow::PleaseWaitWindow()
    : m_frame(FRAME_X, 0, m_width - FRAME_WIDTH_CUT, 0),
      m_okButton(TEXT_OK),
      m_retryButton(TEXT_RETRY),
      m_backButton(TEXT_BACK) {
    m_text.wrapString(TEXT_PLEASE_WAIT, FONT_MESSAGE, m_width - TEXT_WIDTH_CUT);
    int textHeight = m_text.getWrappedTextHeight();
    int frameY = (m_height - (textHeight + FRAME_EXTRA_WAIT)) >> 1;
    m_frame.setY(frameY);
    m_frame.setHeight(textHeight + FRAME_EXTRA_WAIT);
    m_okButton.setPosition(OFFSCREEN, OFFSCREEN);
    m_backButton.setPosition(OFFSCREEN, OFFSCREEN);
    m_retryButton.setPosition(OFFSCREEN, OFFSCREEN);
    NetworkWaitEffect* spinner = AppEngine::getCanvas()->getWindowStore().getNetworkWaitEffect();
    spinner->play(m_width >> 1, textHeight + frameY + SPINNER_BELOW);
}

PleaseWaitWindow::~PleaseWaitWindow() {}

}
