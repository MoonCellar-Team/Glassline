#include "game/UI/ConfirmationWindow.h"

static const int NO_BUTTON = -1;
static const int FRAME_MARGIN_X = 0x6e;
static const int FRAME_WIDTH_CUT = 0xdc;
static const int TEXT_WIDTH_CUT = 0x1b8;
static const int FRAME_EXTRA_HEIGHT = 0xdc;
static const int BUTTON_RIGHT = 0x3c;
static const int BUTTON_GAP = 0x10;
static const int BUTTON_BOTTOM = 0x28;
static const int TEXT_INSET = 0x6e;
static const int TEXT_ANCHOR = 9;
static const int FONT_MESSAGE = 2;

namespace UI {

bool ConfirmationWindow::pointerPressed(int x, int y, int id) {
    if (m_leftButton.contains(x, y) && m_leftButton.getStringId() != NO_BUTTON) {
        m_leftButton.pointerPressed(x, y, id);
        return true;
    }
    if (m_rightButton.contains(x, y) && m_rightButton.getStringId() != NO_BUTTON) {
        m_rightButton.pointerPressed(x, y, id);
        return true;
    }
    return false;
}

bool ConfirmationWindow::pointerDragged(int x, int y, int id) {
    if (m_leftButton.isPressed() && !m_leftButton.contains(x, y)) {
        m_leftButton.unpress();
    }
    if (m_rightButton.isPressed() && !m_rightButton.contains(x, y)) {
        m_rightButton.unpress();
    }
    return false;
}

void ConfirmationWindow::render(midp::Graphics* g, int originY, int originX) {
    m_frame.render(g, originY, originX);
    m_text.draw(g, m_frame.getX() + TEXT_INSET, m_frame.getY() + TEXT_INSET, TEXT_ANCHOR);
    if (m_leftButton.getStringId() != NO_BUTTON) {
        m_leftButton.render(g, originY, originX);
    }
    if (m_rightButton.getStringId() != NO_BUTTON) {
        m_rightButton.render(g, originY, originX);
    }
}

bool ConfirmationWindow::pointerReleased(int x, int y, int id) {
    if (m_leftButton.contains(x, y) && m_leftButton.getStringId() != NO_BUTTON) {
        m_leftButton.pointerReleased(x, y, id);
        close(RESULT_OK);
        return true;
    }
    if (m_leftButton.isPressed()) {
        m_leftButton.unpress();
    }
    if (m_rightButton.contains(x, y) && m_rightButton.getStringId() != NO_BUTTON) {
        m_rightButton.pointerReleased(x, y, id);
        close(RESULT_CANCEL);
        return true;
    }
    if (m_rightButton.isPressed()) {
        m_rightButton.unpress();
    }
    return false;
}

void ConfirmationWindow::sharedConstruction(int message) {
    m_text.wrapString(message, m_font, m_width - TEXT_WIDTH_CUT);
    int textHeight = m_text.getWrappedTextHeight();
    m_frame.setY((m_height - (textHeight + FRAME_EXTRA_HEIGHT)) >> 1);
    m_frame.setHeight(textHeight + FRAME_EXTRA_HEIGHT);
    int buttonY = (m_height - m_rightButton.getHeight()) - BUTTON_BOTTOM;
    int rightX = (m_width - m_rightButton.getWidth()) - BUTTON_RIGHT;
    m_rightButton.setPosition(rightX, buttonY);
    m_leftButton.setPosition(rightX - BUTTON_GAP - m_leftButton.getWidth(), buttonY);
}

ConfirmationWindow::ConfirmationWindow(int message, int rightButton)
    : m_frame(FRAME_MARGIN_X, 0, m_width - FRAME_WIDTH_CUT, 0),
      m_font(FONT_MESSAGE),
      m_leftButton(NO_BUTTON),
      m_rightButton(rightButton) {
    sharedConstruction(message);
}

ConfirmationWindow::ConfirmationWindow(int message, int leftButton, int rightButton)
    : m_frame(FRAME_MARGIN_X, 0, m_width - FRAME_WIDTH_CUT, 0),
      m_font(FONT_MESSAGE),
      m_leftButton(leftButton),
      m_rightButton(rightButton) {
    sharedConstruction(message);
}

ConfirmationWindow::~ConfirmationWindow() {}

}
