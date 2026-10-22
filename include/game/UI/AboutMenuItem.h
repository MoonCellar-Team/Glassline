#pragma once

#include "game/UI/WindowElement.h"

namespace midp {
class Graphics;
}

namespace UI {

class AboutMenuItem : public WindowElement {
public:
    explicit AboutMenuItem(int stringId);
    virtual ~AboutMenuItem();

    virtual void render(midp::Graphics* g, int originY, int originX);

    int getStringId() const;

    int m_stringId;
};

}
