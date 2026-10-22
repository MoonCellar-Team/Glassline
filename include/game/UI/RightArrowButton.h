#pragma once

#include "game/UI/Button.h"

namespace midp {
class Graphics;
}

namespace UI {

class RightArrowButton : public Button {
public:
    RightArrowButton();
    virtual ~RightArrowButton();

    virtual void render(midp::Graphics* g, int originY, int originX);
};

}
