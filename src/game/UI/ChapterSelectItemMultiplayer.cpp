#include "game/UI/ChapterSelectItemMultiplayer.h"

namespace UI {

void ChapterSelectItemMultiplayer::render(midp::Graphics* g, int originY, int originX) {
    ChapterSelectItem::render(g, originY, originX);
}

ChapterSelectItemMultiplayer::ChapterSelectItemMultiplayer(const Level& level)
    : ChapterSelectItem(level) {}

ChapterSelectItemMultiplayer::~ChapterSelectItemMultiplayer() {}

}
