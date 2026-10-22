#include "game/GameUI/SingleQuad.h"

#include "game/QuadManager.h"

namespace GameUI {

static const int ANCHOR_LEFT_TOP = 9;
static const int QUAD_SCOPE = 2;

int SingleQuad::getX() {
    return m_x;
}

int SingleQuad::getY() {
    return m_y;
}

SingleQuad::SingleQuad(int mesh) : m_hasPosition(false), m_mesh(mesh), m_x(0), m_y(0), m_anchor(ANCHOR_LEFT_TOP) {}

SingleQuad::~SingleQuad() {}

void SingleQuad::set(int x, int y, int anchor) {
    m_hasPosition = true;
    m_x = x;
    m_y = y;
    m_anchor = anchor;
}

void SingleQuad::render3D(m3g::Graphics3D* g3d, QuadManager& quads) {
    if (!m_hasPosition) {
        return;
    }
    int width = quads.getMeshWidth(m_mesh);
    int height = quads.getMeshHeight(m_mesh);
    quads.setMeshBounds(m_mesh, m_x, m_y, width, height, m_anchor);
    quads.setMeshVisible(m_mesh, true);
    quads.render(g3d, QUAD_SCOPE);
    quads.setMeshVisible(m_mesh, false);
}

}
