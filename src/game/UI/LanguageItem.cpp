#include "game/UI/LanguageItem.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"

static const int ITEM_WIDTH = 270;
static const int FONT_ITEM = 4;
static const int ANCHOR_CENTER = 18;

namespace UI {

void LanguageItem::render(midp::Graphics* g, int originY, int originX) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int x = originX + m_x + (m_width >> 1);
    int y = originY + m_y + (m_height >> 1);
    text->drawString(g, m_name, FONT_ITEM, x, y, ANCHOR_CENTER);
}

LanguageItem::LanguageItem(int langId, midp::ReferenceCountedPointer<midp::String> name)
    : m_langId(langId),
      m_name(name) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    setWidth(ITEM_WIDTH);
    setHeight(text->getLineHeight(FONT_ITEM));
}

int LanguageItem::getLangId() const {
    return m_langId;
}

LanguageItem::~LanguageItem() {}

}
