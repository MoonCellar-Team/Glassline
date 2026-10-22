#pragma once

#include "game/UI/ChapterSelectWindow.h"

namespace UI {

class ChapterSelectStoryWindow : public ChapterSelectWindow {
public:
    ChapterSelectStoryWindow();
    virtual ~ChapterSelectStoryWindow();

    virtual void onSelected();
};

}
