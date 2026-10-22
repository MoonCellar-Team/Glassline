#pragma once

#include "game/UI/LeftArrowButton.h"
#include "game/UI/NotchedSlider.h"
#include "game/UI/RightArrowButton.h"
#include "game/UI/TitledWindow.h"

namespace UI {

class ChapterSelectWindow : public TitledWindow {
public:
    explicit ChapterSelectWindow(int subtitle);
    virtual ~ChapterSelectWindow();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);
    virtual void onSelected() = 0;

    MajorButton m_startButton;
    LeftArrowButton m_prevButton;
    RightArrowButton m_nextButton;
    NotchedSlider m_chapters;
};

}
