#pragma once

#include "game/UI/SliderElement.h"

namespace midp {
class Graphics;
}

namespace UI {

class HorizontalSliderElement : public SliderElement {
public:
    explicit HorizontalSliderElement(int width);
    virtual ~HorizontalSliderElement();

    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);
};

}
