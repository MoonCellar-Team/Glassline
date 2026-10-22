#pragma once

#include <memory>

#include "game/UI/TitledWindow.h"

namespace UI {

class HelpWindow : public TitledWindow {
public:
    HelpWindow();
    virtual ~HelpWindow();

    virtual void update(int dt);
    using TitledWindow::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    std::auto_ptr<Window> m_panel;
};

}
