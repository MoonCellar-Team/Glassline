#include "game/UI/AboutMenuItem.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"

static const int ITEM_WIDTH = 560;
static const int FONT_ITEM = 4;
static const int ANCHOR_CENTER = 18;

namespace UI {

void AboutMenuItem::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int x = originX + m_x + (m_width >> 1);
    int y = originY + m_y + (m_height >> 1);
    text->drawString(g, m_stringId, FONT_ITEM, x, y, ANCHOR_CENTER);
}

AboutMenuItem::AboutMenuItem(int stringId) : m_stringId(stringId) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    setWidth(ITEM_WIDTH);
    setHeight(text->getLineHeight(FONT_ITEM));
}

int AboutMenuItem::getStringId() const {
    return m_stringId;
}

AboutMenuItem::~AboutMenuItem() {}

}
