#pragma once

#include "game/UI/NotchedSlider.h"

namespace UI {

class AboutMenu;

class AboutMenuPanel : public NotchedSlider {
public:
    explicit AboutMenuPanel(AboutMenu* menu);
    virtual ~AboutMenuPanel();

    virtual bool pointerReleased(int x, int y, int id);
};

}
