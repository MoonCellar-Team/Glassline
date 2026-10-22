#include "game/GameUI/BottomButtonSet.h"

#include "game/GameCommon.h"
#include "game/GameScreen.h"

namespace GameUI {

static const int BUTTON_MARGIN = 30;

BottomButtonSet::~BottomButtonSet() {}

BottomButtonSet::BottomButtonSet(int labelId, GameScreen* screen)
    : m_screen(screen),
      m_button(Button::QUAD_GROUP, Button::QUAD_MESH, labelId),
      m_secondButton(-1, -1, Button::LABEL_NONE) {
    int x = screen->getLocalWidth() - BUTTON_MARGIN - m_button.getWidth();
    int y = screen->getLocalHeight() - BUTTON_MARGIN - m_button.getHeight();
    m_button.setTopLeft(x, y);
}

int BottomButtonSet::pointerReleased(int pressX, int pressY, int x, int y) {
    if (m_button.pointerReleased(pressX, pressY, x, y, m_screen)) {
        return RESULT_BUTTON;
    }
    if (m_secondButton.getLabelId() == Button::LABEL_NONE) {
        return RESULT_NONE;
    }
    return m_secondButton.pointerReleased(pressX, pressY, x, y, m_screen) ? RESULT_SECOND_BUTTON : RESULT_NONE;
}

void BottomButtonSet::pointerDragged(int pressX, int pressY, int x, int y) {
    m_button.pointerDragged(pressX, pressY, x, y);
    if (m_secondButton.getLabelId() != Button::LABEL_NONE) {
        m_secondButton.pointerDragged(pressX, pressY, x, y);
    }
}

void BottomButtonSet::pointerPressed(int x, int y) {
    m_button.pointerPressed(x, y);
    if (m_secondButton.getLabelId() != Button::LABEL_NONE) {
        m_secondButton.pointerPressed(x, y);
    }
}

void BottomButtonSet::render2D(midp::Graphics* g) {
    m_button.render2D(g);
    m_secondButton.render2D(g);
}

void BottomButtonSet::render3D(m3g::Graphics3D* g3d) {
    m_button.render3D(g3d);
    m_secondButton.render3D(g3d);
}

}
