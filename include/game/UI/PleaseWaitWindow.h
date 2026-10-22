#pragma once

#include "game/UI/BorderedElement.h"
#include "game/UI/MajorButton.h"
#include "game/UI/Window.h"
#include "game/WrappedString.h"

namespace UI {

class PleaseWaitWindow : public Window {
public:
    PleaseWaitWindow();
    virtual ~PleaseWaitWindow();

    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    enum Button {
        BUTTON_RETRY = 1,
        BUTTON_BACK = 2,
        BUTTON_OK = 3,
    };

    void Succeeded(int textId);
    void Failed(int textId);

    BorderedElement m_frame;
    WrappedString m_text;
    MajorButton m_okButton;
    MajorButton m_retryButton;
    MajorButton m_backButton;
};

}
