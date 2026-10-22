#pragma once

#include "game/UI/WindowElement.h"

namespace midp {
class Graphics;
}

namespace UI {

class NotchedItemStringId : public WindowElement {
public:
    explicit NotchedItemStringId(int stringId);
    virtual ~NotchedItemStringId();

    virtual void render(midp::Graphics* g, int originY, int originX);

    int getStringId() const;

    int m_stringId;
};

}
