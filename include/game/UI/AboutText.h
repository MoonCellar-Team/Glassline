#pragma once

#include <memory>

#include "game/UI/WindowElement.h"
#include "game/WrappedString.h"

namespace UI {

class AboutText : public WindowElement {
public:
    explicit AboutText(const std::auto_ptr<Window>& window);
    virtual ~AboutText();

    virtual void render(midp::Graphics* g, int originY, int originX);

    WrappedString m_text;
};

}
