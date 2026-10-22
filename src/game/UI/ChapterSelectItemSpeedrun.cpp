#include "game/UI/ChapterSelectItemSpeedrun.h"

#include "game/AppEngine.h"
#include "game/Level.h"

static const int STAT_DX = 4;
static const int STAT_DY = 25;
static const int FONT_STAT = 2;
static const int TEXT_TIME = 0x83f;
static const int ANCHOR_STAT = 65;
static const AppEngine::StatType STAT_BEST_TIME = static_cast<AppEngine::StatType>(4);

namespace UI {

void ChapterSelectItemSpeedrun::render(midp::Graphics* g, int originY, int originX) {
    ChapterSelectItem::render(g, originY, originX);
    AppEngine* canvas = AppEngine::getCanvas();
    int best = m_level->getBestSpeedRunTimeMillis();
    int x = originX + m_x + STAT_DX;
    int y = originY + m_y + m_height + STAT_DY;
    canvas->drawStatString(g, FONT_STAT, TEXT_TIME, STAT_BEST_TIME, best, x, y, ANCHOR_STAT);
}

ChapterSelectItemSpeedrun::ChapterSelectItemSpeedrun(const Level& level) : ChapterSelectItem(level) {}

ChapterSelectItemSpeedrun::~ChapterSelectItemSpeedrun() {}

}
