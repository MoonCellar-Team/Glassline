#include "game/UI/ChapterSelectItem.h"

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"

static const int ITEM_WIDTH = 250;
static const int ITEM_HEIGHT = 40;
static const int FONT_ITEM = 9;
static const int ANCHOR_ITEM = 9;

namespace UI {

void ChapterSelectItem::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    text->drawString(g, m_name, FONT_ITEM, originX + m_x + 4, originY + m_y + 5, ANCHOR_ITEM);
}

ChapterSelectItem::ChapterSelectItem(const Level& level)
    : m_level(&level),
      m_name(NULL) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    midp::String* name = text->getString(level.getName());
    m_name = name->toUpperCase();
    setWidth(ITEM_WIDTH);
    setHeight(ITEM_HEIGHT);
}

const Level* ChapterSelectItem::getLevelObject() const {
    return m_level;
}

ChapterSelectItem::~ChapterSelectItem() {}

}
