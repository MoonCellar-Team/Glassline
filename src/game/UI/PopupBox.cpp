#include "game/UI/PopupBox.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "runtime/midp/Graphics.h"

static const int FONT_BODY = 2;
static const int FONT_HINT = 3;
static const int TEXT_HINT = 0x813;
static const int TEXT_INSET_X = 8;
static const int TEXT_INSET_Y = 12;
static const int TEXT_ANCHOR = 9;
static const int HINT_ANCHOR = 0x24;
static const int ELAPSED_LIMIT = 1000;
static const int FRAME_PAD_X = 0x10;
static const int FRAME_PAD_Y = 0x11;

namespace UI {

void PopupBox::update(int dt) {
    if (m_elapsed <= ELAPSED_LIMIT) {
        m_elapsed += dt;
    }
}

bool PopupBox::pointerReleased(int x, int y, int id) {
    if (m_elapsed < ELAPSED_LIMIT) {
        return false;
    }
    close(RESULT_OK);
    return true;
}

void PopupBox::render(midp::Graphics* g, int originY, int originX) {
    g->setColor(0);
    g->fillRect(0, 0, 1, 1);
    m_frame.render(g, originY, originX);
    m_text.draw(g, TEXT_INSET_X, TEXT_INSET_Y, TEXT_ANCHOR);
    if (m_elapsed >= ELAPSED_LIMIT) {
        TextManager* text = AppEngine::getCanvas()->getTextManager();
        int x = m_width - 5;
        int y = m_frame.getY() + m_frame.getHeight() - 5;
        text->drawString(g, TEXT_HINT, FONT_HINT, x, y, HINT_ANCHOR);
    }
}

PopupBox::PopupBox(int text)
    : m_frame(0, 0, 0, 0),
      m_elapsed(0) {
    m_text.wrapString(text, FONT_BODY, m_width - FRAME_PAD_X);
    TextManager* labels = AppEngine::getCanvas()->getTextManager();
    int height = labels->getLineHeight(FONT_BODY) + m_text.getWrappedTextHeight() + FRAME_PAD_Y;
    m_frame.setDimensions(m_width, height);
}

PopupBox::~PopupBox() {}

}
