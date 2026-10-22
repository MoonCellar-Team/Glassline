#pragma once

#include "game/UI/BorderedElement.h"
#include "game/UI/MajorButton.h"
#include "game/UI/Window.h"
#include "game/WrappedString.h"

namespace UI {

class UploadConfirmationWindow : public Window {
public:
    UploadConfirmationWindow();
    virtual ~UploadConfirmationWindow();

    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    BorderedElement m_frame;
    int m_font;
    WrappedString m_text;
    MajorButton m_noButton;
    MajorButton m_yesButton;
    MajorButton m_hiddenYes;
    int m_phase;
};

}
