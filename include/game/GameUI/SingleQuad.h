#pragma once

class QuadManager;
namespace m3g {
class Graphics3D;
}

namespace GameUI {

class SingleQuad {
public:
    explicit SingleQuad(int mesh);
    ~SingleQuad();
    void set(int x, int y, int anchor);
    void render3D(m3g::Graphics3D* g3d, QuadManager& quads);
    int getX();
    int getY();

    bool m_hasPosition;
    int m_mesh;
    int m_x;
    int m_y;
    int m_anchor;
};

}
