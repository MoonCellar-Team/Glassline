#include "game/GameUI/SolidRect.h"

#include "game/AppEngine.h"
#include "game/QuadManager.h"

namespace GameUI {

static const int MESH_RECT = 120;
static const int MESH_ANCHOR = 9;
static const int QUAD_SCOPE = 2;

static int opaqueIfNoAlpha(int color) {
    return (unsigned int)color < 0x1000000u ? (int)(color | 0xff000000u) : color;
}

SolidRect::SolidRect(int color)
    : m_hasBounds(false), m_x(0), m_y(0), m_width(0), m_height(0), m_color(opaqueIfNoAlpha(color)) {}

SolidRect::SolidRect(int x, int y, int width, int height, int color)
    : m_hasBounds(true), m_x(x), m_y(y), m_width(width), m_height(height), m_color(opaqueIfNoAlpha(color)) {}

void SolidRect::setBounds(int x, int y, int width, int height) {
    m_hasBounds = true;
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void SolidRect::render3D(m3g::Graphics3D* g3d) {
    if (!m_hasBounds) {
        return;
    }
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
#ifdef MEDECOMP_PORT
    AppEngine* canvas = AppEngine::getCanvas();
    const int extra = medecomp_wide_extra();
    if (extra != 0 && m_x == 0 && m_y == 0 && m_width == canvas->getWidth() && m_height == canvas->getHeight()) {
        quads.setMeshBounds(MESH_RECT, -extra, 0, m_width + 2 * extra, m_height, MESH_ANCHOR);
    } else
#endif
    quads.setMeshBounds(MESH_RECT, m_x, m_y, m_width, m_height, MESH_ANCHOR);
    quads.setMeshColor(MESH_RECT, m_color);
    quads.setMeshVisible(MESH_RECT, true);
    quads.render(g3d, QUAD_SCOPE);
    quads.setMeshVisible(MESH_RECT, false);
}

int SolidRect::getTop() {
    return m_y;
}

int SolidRect::getHeight() {
    return m_height;
}

void SolidRect::setTop(int y) {
    m_y = y;
}

}
