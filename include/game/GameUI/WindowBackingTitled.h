#pragma once

#include "game/GameUI/WindowBacking.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
class String;
}

namespace GameUI {

class WindowBackingTitled {
public:
    WindowBackingTitled(int titleId, int headerId);
    ~WindowBackingTitled();
    void render2D(midp::Graphics* g);
    void render3D(m3g::Graphics3D* g3d);
    void setBounds(int left, int top, int width, int height, int withTitle);
    int getTop();
    int getHeight();

    midp::ReferenceCountedPointer<midp::String> m_title;
    midp::ReferenceCountedPointer<midp::String> m_header;
    int m_titleY;
    int m_headerY;
    int m_titleHeight;
    WindowBacking m_backing;
};

}
