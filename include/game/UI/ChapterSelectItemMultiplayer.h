#pragma once

#include "game/UI/ChapterSelectItem.h"

namespace midp {
class Graphics;
}

namespace UI {

class ChapterSelectItemMultiplayer : public ChapterSelectItem {
public:
    explicit ChapterSelectItemMultiplayer(const Level& level);
    virtual ~ChapterSelectItemMultiplayer();

    virtual void render(midp::Graphics* g, int originY, int originX);
};

}
