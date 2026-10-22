#pragma once

#include "game/UI/ChapterSelectItem.h"

namespace midp {
class Graphics;
}

namespace UI {

class ChapterSelectItemStory : public ChapterSelectItem {
public:
    explicit ChapterSelectItemStory(const Level& level);
    virtual ~ChapterSelectItemStory();

    virtual void render(midp::Graphics* g, int originY, int originX);
};

}
