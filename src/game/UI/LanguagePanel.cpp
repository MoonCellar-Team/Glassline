#include "game/UI/LanguagePanel.h"

#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "game/UI/LanguageItem.h"
#include "runtime/midp/String.h"

static const int PANEL_WIDTH = 0x14a;
static const int PANEL_HEIGHT = 0x50;
static const int RENDER_EXTRA = 2;
static const int TEXT_LANGUAGE_NAME = 0;

namespace UI {

LanguagePanel::LanguagePanel() {
    setWidth(PANEL_WIDTH);
    setHeight(PANEL_HEIGHT);

    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int count = text->getLanguageCount();
    int maxWidth = 0;
    for (int language = 0; language < count; language++) {
        midp::ReferenceCountedPointer<midp::String> name(
            text->getLangString(TEXT_LANGUAGE_NAME, language));
        LanguageItem* item = new LanguageItem(language, name);
        int width = item->getWidth();
        if (width > maxWidth) {
            maxWidth = item->getWidth();
        }
        addItem(item);
    }
    setNotchWidth(maxWidth);
    setRenderExtra(RENDER_EXTRA);
    int current = text->getCurrentLanguage();
    m_halfNotchWidth = (float)(current * maxWidth + (maxWidth >> 1));
}

LanguagePanel::~LanguagePanel() {}

}
