#include "game/UI/AboutText.h"

#include "game/UI/Window.h"

static const int WIDTH_TRIM = 21;
static const int WRAP_TRIM = 10;
static const int TEXT_ABOUT = -11;
static const int FONT_TEXT = 2;
static const int DRAW_DX = 5;
static const int ANCHOR_TEXT = 9;

namespace UI {

void AboutText::render(midp::Graphics* g, int originY, int originX) {
    m_text.draw(g, originX + m_x + DRAW_DX, originY + m_y, ANCHOR_TEXT);
}

AboutText::AboutText(const std::auto_ptr<Window>& window)
    : WindowElement(0, 0, window->getClientWidth() - WIDTH_TRIM, 0) {
    m_text.wrapString(TEXT_ABOUT, FONT_TEXT, m_width - WRAP_TRIM);
    setHeight(m_text.getWrappedTextHeight());
}

AboutText::~AboutText() {}

}
