#include "game/GameUI/WindowBacking.h"

#include "game/AppEngine.h"
#include "game/QuadManager.h"

namespace GameUI {

static const int QUAD_GROUP = 71;
static const int ANIM_WIDTH = 18;
static const int ANIM_HEIGHT = 19;
static const int QUAD_SCOPE = 2;

int WindowBacking::getLeft() {
    return m_left;
}

int WindowBacking::getTop() {
    return m_top;
}

WindowBacking::WindowBacking() : m_left(-1), m_top(-1), m_width(-1), m_height(-1) {}

void WindowBacking::setBounds(int left, int top, int width, int height) {
    m_left = left;
    m_top = top;
    m_width = width;
    m_height = height;
}

bool WindowBacking::isPointWithin(int x, int y) {
    return x >= m_left && x <= m_left + m_width && y >= m_top && y <= m_top + m_height;
}

void WindowBacking::render3D(m3g::Graphics3D* g3d) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    quads.setGroupVisible(QUAD_GROUP, true);
    quads.setGroupPosition(QUAD_GROUP, m_left, m_top);
    quads.setAnimTime(ANIM_WIDTH, m_width);
    quads.setAnimTime(ANIM_HEIGHT, m_height);
    quads.render(g3d, QUAD_SCOPE);
    quads.setGroupVisible(QUAD_GROUP, false);
}

int WindowBacking::getHeight() {
    return m_height;
}

}
