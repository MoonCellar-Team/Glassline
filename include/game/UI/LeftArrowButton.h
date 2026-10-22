#pragma once

#include "game/UI/Button.h"

namespace midp {
class Graphics;
}

namespace UI {

class LeftArrowButton : public Button {
public:
    LeftArrowButton();
    virtual ~LeftArrowButton();

    virtual void render(midp::Graphics* g, int originY, int originX);
};

}
