#pragma once

#include "game/UI/BorderedElement.h"
#include "game/UI/MajorButton.h"
#include "game/UI/Window.h"
#include "game/WrappedString.h"

namespace UI {

class ConfirmationWindow : public Window {
public:
    ConfirmationWindow(int message, int rightButton);
    ConfirmationWindow(int message, int leftButton, int rightButton);
    virtual ~ConfirmationWindow();

    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    void sharedConstruction(int message);

    BorderedElement m_frame;
    int m_font;
    WrappedString m_text;
    MajorButton m_leftButton;
    MajorButton m_rightButton;
};

}
