#pragma once

#include "game/UI/ChapterSelectWindow.h"

namespace UI {

class ChapterSelectSpeedrunWindow : public ChapterSelectWindow {
public:
    ChapterSelectSpeedrunWindow();
    virtual ~ChapterSelectSpeedrunWindow();

    using ChapterSelectWindow::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual void onSelected();
};

}
