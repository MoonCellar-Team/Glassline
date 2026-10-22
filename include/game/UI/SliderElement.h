#pragma once

#include "game/UI/WindowElement.h"

namespace midp {
class Graphics;
}

namespace UI {

class SliderElement : public WindowElement {
public:
    SliderElement(int width, int height);
    virtual ~SliderElement();

    virtual void update(int dt);

    float getValue() const;
    void setValue(float value);

    bool m_dragging;
    float m_previous;
    float m_value;
    float m_velocity;
};

}
