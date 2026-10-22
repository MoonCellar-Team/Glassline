#pragma once

#include "game/UI/WindowElement.h"

namespace midp {
class DataInputStream;
class Graphics;
}

namespace UI {

class LoadingScreenImageElement : public WindowElement {
public:
    LoadingScreenImageElement(midp::DataInputStream& in, int unused);
    virtual ~LoadingScreenImageElement();

    virtual void render(midp::Graphics* g, int originY, int originX);

    int m_imageId;
    int m_anchor;
};

}
