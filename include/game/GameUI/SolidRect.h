#pragma once

namespace m3g {
class Graphics3D;
}

namespace GameUI {

class SolidRect {
public:
    explicit SolidRect(int color);
    SolidRect(int x, int y, int width, int height, int color);
    void setBounds(int x, int y, int width, int height);
    void render3D(m3g::Graphics3D* g3d);
    int getTop();
    int getHeight();
    void setTop(int y);

    bool m_hasBounds;
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    int m_color;
};

}
