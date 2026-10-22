#pragma once

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class GameScreen;
namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
class String;
}

namespace GameUI {

class Button : public midp::ReferenceCounted {
public:
    static const int LABEL_NONE = 0x800;
    static const int QUAD_GROUP = 69;
    static const int QUAD_MESH = 136;

    Button(int group, int mesh, int labelId);
    ~Button();
    void setTopLeft(int x, int y);
    bool isPointerOnButton(int x, int y);
    bool pointerReleased(int pressX, int pressY, int x, int y, GameScreen* screen);
    void pointerDragged(int pressX, int pressY, int x, int y);
    void pointerPressed(int x, int y);
    void render2D(midp::Graphics* g);
    void render3D(m3g::Graphics3D* g3d);
    int getHeight();
    int getLabelId();
    int getWidth();

    int m_x;
    int m_y;
    int m_labelId;
    midp::ReferenceCountedPointer<midp::String> m_label;
    int m_group;
    int m_mesh;
    int m_width;
    bool m_pressed;
};

}
