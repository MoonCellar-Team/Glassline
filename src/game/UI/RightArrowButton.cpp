#include "game/UI/RightArrowButton.h"

#include "game/AppEngine.h"
#include "game/QuadManager.h"
#include "runtime/midp/Graphics.h"

static const int GROUP_ARROWS = 0x37;
static const int MESH_RIGHT = 0x82;
static const int MESH_RIGHT_DISABLED = 0x84;
static const int SCOPE_UI = 2;

namespace UI {

void RightArrowButton::render(midp::Graphics* g, int originY, int originX) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    int mesh = m_enabled ? MESH_RIGHT : MESH_RIGHT_DISABLED;
    quads.setGroupVisible(GROUP_ARROWS, true);
    quads.setMeshVisible(mesh, true);
    quads.setMeshBounds(mesh, (float)(m_x + originX), (float)(m_y + originY), (float)m_width,
                        (float)m_height, QuadManager::ANCHOR_LEFT | QuadManager::ANCHOR_TOP);
    quads.render(g, SCOPE_UI);
    quads.setMeshVisible(mesh, false);
    quads.setGroupVisible(GROUP_ARROWS, false);
}

RightArrowButton::RightArrowButton() {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    setWidth(quads.getMeshWidth(MESH_RIGHT));
    setHeight(quads.getMeshHeight(MESH_RIGHT));
}

RightArrowButton::~RightArrowButton() {}

}
