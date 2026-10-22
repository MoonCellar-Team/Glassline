#include "game/GameUI/Confirmation.h"

#include "game/GameCommon.h"

namespace GameUI {

static const int BUTTON_MARGIN = 30;
static const int BUTTON_GAP = 16;
static const int WINDOW_MARGIN = 120;
static const int TEXT_PADDING = 22;
static const int FONT_TEXT = 2;
static const int ANCHOR_LEFT_TOP = 9;

Confirmation::Confirmation(int textId, int width, int height, int yesId, int noId)
    : m_backing(),
      m_text(),
      m_yes(Button::QUAD_GROUP, Button::QUAD_MESH, yesId),
      m_no(Button::QUAD_GROUP, Button::QUAD_MESH, noId) {
    setupWindow(textId, width, height);
    int noWidth = m_no.getWidth();
    int noX = width - BUTTON_MARGIN - noWidth;
    int yesWidth = m_yes.getWidth();
    int y = height - BUTTON_MARGIN - m_no.getHeight();
    m_no.setTopLeft(noX, y);
    m_yes.setTopLeft(width - BUTTON_MARGIN - BUTTON_GAP - (yesWidth + noWidth), y);
}

int Confirmation::pointerReleased(int pressX, int pressY, int x, int y, GameScreen* screen) {
    if (m_yes.pointerReleased(pressX, pressY, x, y, screen)) {
        return RESULT_YES;
    }
    if (m_no.getLabelId() == Button::LABEL_NONE) {
        return RESULT_NONE;
    }
    return m_no.pointerReleased(pressX, pressY, x, y, screen) ? RESULT_NO : RESULT_NONE;
}

void Confirmation::pointerDragged(int pressX, int pressY, int x, int y) {
    m_yes.pointerDragged(pressX, pressY, x, y);
    if (m_no.getLabelId() != Button::LABEL_NONE) {
        m_no.pointerDragged(pressX, pressY, x, y);
    }
}

void Confirmation::pointerPressed(int x, int y) {
    m_yes.pointerPressed(x, y);
    if (m_no.getLabelId() != Button::LABEL_NONE) {
        m_no.pointerPressed(x, y);
    }
}

void Confirmation::render2D(midp::Graphics* g) {
    int x = m_backing.getLeft() + TEXT_PADDING;
    int y = m_backing.getTop() + TEXT_PADDING;
    m_text.draw(g, x, y, ANCHOR_LEFT_TOP);
    m_yes.render2D(g);
    m_no.render2D(g);
}

void Confirmation::render3D(m3g::Graphics3D* g3d) {
    m_backing.render3D(g3d);
    m_yes.render3D(g3d);
    m_no.render3D(g3d);
}

void Confirmation::setupWindow(int textId, int width, int height) {
    m_text.wrapString(textId, FONT_TEXT, width - 2 * WINDOW_MARGIN - 2 * TEXT_PADDING);
    int windowHeight = m_text.getWrappedTextHeight() + 2 * TEXT_PADDING;
    m_backing.setBounds(WINDOW_MARGIN, (height - windowHeight) >> 1, width - 2 * WINDOW_MARGIN, windowHeight);
}

Confirmation::~Confirmation() {}

}
