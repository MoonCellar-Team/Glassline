#pragma once

#include "game/UI/MajorButton.h"
#include "game/UI/Window.h"

namespace UI {

class EndOfLevelPrompt : public Window {
public:
    EndOfLevelPrompt();
    virtual ~EndOfLevelPrompt();

    virtual void update(int dt);
    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX) = 0;
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);
    virtual void UnHide() = 0;

    MajorButton m_restart;
    MajorButton m_continue;
    MajorButton m_next;
    bool m_story;
    bool m_lastStory;
    bool m_newlyCompleted;
    bool m_closing;
};

}
