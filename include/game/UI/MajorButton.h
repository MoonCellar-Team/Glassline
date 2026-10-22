#pragma once

#include "game/UI/Button.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class String;
}

namespace UI {

class MajorButton : public Button {
public:
    explicit MajorButton(int text);
    virtual ~MajorButton();

    virtual void render(midp::Graphics* g, int originY, int originX);

    int m_language;
    midp::ReferenceCountedPointer<midp::String> m_label;
};

}
