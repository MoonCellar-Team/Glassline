#pragma once

#include "game/UI/BorderedElement.h"
#include "game/UI/MajorButton.h"
#include "game/UI/Window.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}

namespace UI {

class TitledWindow : public Window {
public:
    TitledWindow(int title, int subtitle);
    virtual ~TitledWindow();

    virtual void update(int dt);
    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    void setTitles(int title, int subtitle);
    void setShowBackground(bool show);

    midp::ReferenceCountedPointer<midp::String> m_title;
    midp::ReferenceCountedPointer<midp::String> m_subtitle;
    BorderedElement m_background;
    MajorButton m_backButton;
    bool m_showBackground;
};

}
