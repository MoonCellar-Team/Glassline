#include "game/UI/NotchedItemStringId.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"

static const int ITEM_WIDTH = 250;
static const int ITEM_HEIGHT = 40;
static const int FONT_ITEM = 9;
static const int ANCHOR_ITEM = 9;

namespace UI {

void NotchedItemStringId::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    text->drawString(g, m_stringId, FONT_ITEM, originX + m_x + 4, originY + m_y + 5, ANCHOR_ITEM);
}

NotchedItemStringId::NotchedItemStringId(int stringId)
    : m_stringId(stringId) {
    setWidth(ITEM_WIDTH);
    setHeight(ITEM_HEIGHT);
}

int NotchedItemStringId::getStringId() const {
    return m_stringId;
}

NotchedItemStringId::~NotchedItemStringId() {}

}
