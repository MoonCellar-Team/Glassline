#pragma once

#include "game/UI/AchievementsList.h"
#include "game/UI/TitledWindow.h"

namespace UI {

class AchievementWindow : public TitledWindow {
public:
    AchievementWindow();
    virtual ~AchievementWindow();

    virtual void update(int dt);
    using TitledWindow::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    AchievementsList m_list;
};

}
