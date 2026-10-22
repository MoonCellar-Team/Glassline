#include "game/UI/UploadConfirmationWindow.h"

#include "game/AppEngine.h"

static const int TEXT_QUESTION = 0x925;
static const int TEXT_PLEASE_WAIT = 0x92a;
static const int TEXT_YES = 0x805;
static const int TEXT_NO = 0x806;
static const int NO_BUTTON = -1;

static const int FONT_MESSAGE = 2;
static const int FRAME_X = 0x2e;
static const int FRAME_WIDTH_CUT = 0x5c;
static const int TEXT_WIDTH_CUT = 0x8e;
static const int FRAME_EXTRA = 0x32;
static const int TEXT_X = 0x14;
static const int TEXT_Y = 0x19;
static const int ANCHOR_QUESTION = 9;
static const int ANCHOR_WAIT = 10;
static const int BUTTON_GAP = 8;
static const int BUTTON_BOTTOM = 5;
static const int HIDDEN_POS = -1000;

static const int PHASE_YES = 2;
static const int PHASE_HIDDEN_YES = 3;

namespace UI {

bool UploadConfirmationWindow::pointerPressed(int x, int y, int id) {
    if (m_yesButton.contains(x, y) && m_yesButton.getStringId() != NO_BUTTON) {
        m_yesButton.pointerPressed(x, y, id);
        return true;
    }
    if (m_hiddenYes.contains(x, y) && m_hiddenYes.getStringId() != NO_BUTTON) {
        m_hiddenYes.pointerPressed(x, y, id);
        return true;
    }
    if (m_noButton.contains(x, y) && m_noButton.getStringId() != NO_BUTTON) {
        m_noButton.pointerPressed(x, y, id);
        return true;
    }
    return false;
}

bool UploadConfirmationWindow::pointerDragged(int x, int y, int id) {
    if (m_yesButton.isPressed() && !m_yesButton.contains(x, y)) {
        m_yesButton.unpress();
    }
    if (m_hiddenYes.isPressed() && !m_hiddenYes.contains(x, y)) {
        m_hiddenYes.unpress();
    }
    if (m_noButton.isPressed() && !m_noButton.contains(x, y)) {
        m_noButton.unpress();
    }
    return false;
}

void UploadConfirmationWindow::render(midp::Graphics* g, int originY, int originX) {
    if (m_phase == 0) {
        m_frame.render(g, originY, originX);
        m_text.draw(g, m_frame.getX() + TEXT_X, m_frame.getY() + TEXT_Y, ANCHOR_QUESTION);
        m_noButton.render(g, originY, originX);
        m_yesButton.render(g, originY, originX);
        m_hiddenYes.render(g, originY, originX);
        return;
    }
    m_text.wrapString(TEXT_PLEASE_WAIT, m_font, m_width - TEXT_WIDTH_CUT);
    int textHeight = m_text.getWrappedTextHeight();
    m_frame.setY((m_height - (textHeight + FRAME_EXTRA)) >> 1);
    m_frame.setHeight(textHeight + FRAME_EXTRA);
    m_frame.render(g, originY, originX);
    m_text.draw(g, m_width >> 1, m_frame.getY() + TEXT_Y, ANCHOR_WAIT);
    AppEngine::getCanvas();
    if (m_phase == 1 || m_phase == 2 || m_phase == 3) {
        close(RESULT_OK);
    }
}

bool UploadConfirmationWindow::pointerReleased(int x, int y, int id) {
    if (m_yesButton.contains(x, y) && m_yesButton.getStringId() != NO_BUTTON) {
        m_phase = PHASE_YES;
        m_yesButton.pointerReleased(x, y, id);
        return true;
    }
    if (m_yesButton.isPressed()) {
        m_yesButton.unpress();
    }
    if (m_hiddenYes.contains(x, y) && m_hiddenYes.getStringId() != NO_BUTTON) {
        m_phase = PHASE_HIDDEN_YES;
        m_hiddenYes.pointerReleased(x, y, id);
        return true;
    }
    if (m_hiddenYes.isPressed()) {
        m_hiddenYes.unpress();
    }
    if (m_noButton.contains(x, y) && m_noButton.getStringId() != NO_BUTTON) {
        m_noButton.pointerReleased(x, y, id);
        close(RESULT_CANCEL);
        return true;
    }
    if (m_noButton.isPressed()) {
        m_noButton.unpress();
    }
    return false;
}

UploadConfirmationWindow::UploadConfirmationWindow()
    : m_frame(FRAME_X, 0, m_width - FRAME_WIDTH_CUT, 0),
      m_font(FONT_MESSAGE),
      m_noButton(TEXT_NO),
      m_yesButton(TEXT_YES),
      m_hiddenYes(TEXT_YES),
      m_phase(0) {
    m_text.wrapString(TEXT_QUESTION, m_font, m_width - TEXT_WIDTH_CUT);
    int textHeight = m_text.getWrappedTextHeight();
    m_frame.setY((m_height - (textHeight + FRAME_EXTRA)) >> 1);
    m_frame.setHeight(textHeight + FRAME_EXTRA);
    int buttonY = (m_height - m_noButton.getHeight()) - BUTTON_BOTTOM;
    int noX = (m_width - BUTTON_GAP) - m_noButton.getWidth();
    m_noButton.setPosition(noX, buttonY);
    AppEngine::getCanvas();
    m_yesButton.setPosition((noX - BUTTON_GAP) - m_yesButton.getWidth(), buttonY);
    m_hiddenYes.setPosition(HIDDEN_POS, HIDDEN_POS);
}

UploadConfirmationWindow::~UploadConfirmationWindow() {}

}
