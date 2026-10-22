#pragma once

#include "game/UI/BorderedElement.h"
#include "game/UI/MajorButton.h"
#include "game/UI/Window.h"
#include "game/WrappedString.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}

namespace UI {

class DisplayNameEntry : public Window {
public:
    DisplayNameEntry();
    virtual ~DisplayNameEntry();

    virtual void update(int dt);
    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    BorderedElement m_frame;
    WrappedString m_prompt;
    midp::ReferenceCountedPointer<midp::String> m_name;
    MajorButton m_backButton;
    MajorButton m_okButton;
    bool m_editing;
};

}
