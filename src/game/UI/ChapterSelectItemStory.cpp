#include "game/UI/ChapterSelectItemStory.h"

#include "game/AppEngine.h"
#include "game/Level.h"

static const int STAT_DX = 4;
static const int STAT_DY = 25;
static const int FONT_STAT = 2;
static const int TEXT_BAGS = 0x841;
static const int ANCHOR_STAT = 65;
static const AppEngine::StatType STAT_EMPTY = static_cast<AppEngine::StatType>(2);

namespace UI {

void ChapterSelectItemStory::render(midp::Graphics* g, int originY, int originX) {
    ChapterSelectItem::render(g, originY, originX);
    AppEngine* canvas = AppEngine::getCanvas();
    if (m_level->getType() == 0) {
        return;
    }
    int x = originX + m_x + STAT_DX;
    int y = originY + m_y + m_height + STAT_DY;
    if (m_level->isLevelComplete()) {
        int found = m_level->getNumBagsFound();
        int total = m_level->getNumTotalBags();
        canvas->drawOfStatString(g, FONT_STAT, TEXT_BAGS, found, total, x, y, ANCHOR_STAT);
    } else {
        canvas->drawStatString(g, FONT_STAT, TEXT_BAGS, STAT_EMPTY, -1, x, y, ANCHOR_STAT);
    }
}

ChapterSelectItemStory::ChapterSelectItemStory(const Level& level) : ChapterSelectItem(level) {}

ChapterSelectItemStory::~ChapterSelectItemStory() {}

}
