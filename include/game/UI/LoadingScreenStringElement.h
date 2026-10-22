#pragma once

#include "game/UI/WindowElement.h"
#include "game/WrappedString.h"

namespace midp {
class DataInputStream;
class Graphics;
}

namespace UI {

class LoadingScreenStringElement : public WindowElement {
public:
    LoadingScreenStringElement(midp::DataInputStream& in, int y);
    virtual ~LoadingScreenStringElement();

    virtual void render(midp::Graphics* g, int originY, int originX);
    bool usesLargeFont();
    bool isCentered();

    int m_textId;
    int m_font;
    int m_anchor;
    WrappedString m_text;
};

}
