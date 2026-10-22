#pragma once

#include "game/UI/MajorButton.h"
#include "game/UI/Window.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Graphics;
class String;
}

namespace UI {

class EndOfGameResultsWindow : public Window {
public:
    EndOfGameResultsWindow();
    virtual ~EndOfGameResultsWindow();

    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    MajorButton m_button;
    midp::ReferenceCountedPointer<midp::String> m_title;
    midp::ReferenceCountedPointer<midp::String> m_chapterHeader;
    midp::ReferenceCountedPointer<midp::String> m_bagsHeader;
};

}
