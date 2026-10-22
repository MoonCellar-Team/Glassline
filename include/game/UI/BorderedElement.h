#pragma once

#include "game/UI/WindowElement.h"

namespace UI {

class BorderedElement : public WindowElement {
public:
    BorderedElement(int x, int y, int width, int height);
    virtual ~BorderedElement();

    virtual void render(midp::Graphics* g, int originY, int originX);
};

}
