#pragma once

#include "game/UI/CheckBox.h"
#include "game/UI/TitledWindow.h"

namespace UI {

class GameOptions : public TitledWindow {
public:
    GameOptions();
    virtual ~GameOptions();

    using TitledWindow::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerReleased(int x, int y, int id);

    CheckBox m_tutorial;
    CheckBox m_upload;
};

}
