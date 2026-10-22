#pragma once

#include <memory>

#include "game/UI/WindowElement.h"
#include "game/WrappedString.h"

namespace UI {

class HelpText : public WindowElement {
public:
    explicit HelpText(const std::auto_ptr<Window>& window);
    virtual ~HelpText();

    virtual void render(midp::Graphics* g, int originY, int originX);

    WrappedString m_text;
};

}
