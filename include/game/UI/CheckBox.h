#pragma once

#include "game/UI/WindowElement.h"

namespace midp {
class Graphics;
}

namespace UI {

class CheckBox : public WindowElement {
public:
    CheckBox();
    virtual ~CheckBox();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerReleased(int x, int y, int id);

    void setChecked(bool checked);
    bool getChecked() const;

    bool m_checked;
};

}
