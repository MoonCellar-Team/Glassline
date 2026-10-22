#pragma once

namespace m3g {
class Graphics3D;
}

namespace GameUI {

class WindowBacking {
public:
    WindowBacking();
    void setBounds(int left, int top, int width, int height);
    bool isPointWithin(int x, int y);
    void render3D(m3g::Graphics3D* g3d);
    int getLeft();
    int getTop();
    int getHeight();

    int m_left;
    int m_top;
    int m_width;
    int m_height;
};

}
