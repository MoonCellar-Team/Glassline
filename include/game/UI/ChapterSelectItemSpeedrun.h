#pragma once

#include "game/UI/ChapterSelectItem.h"

namespace midp {
class Graphics;
}

namespace UI {

class ChapterSelectItemSpeedrun : public ChapterSelectItem {
public:
    explicit ChapterSelectItemSpeedrun(const Level& level);
    virtual ~ChapterSelectItemSpeedrun();

    virtual void render(midp::Graphics* g, int originY, int originX);
};

}
