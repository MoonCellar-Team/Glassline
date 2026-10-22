#include "game/UI/LeftArrowButton.h"

#include "game/AppEngine.h"
#include "game/QuadManager.h"
#include "runtime/midp/Graphics.h"

static const int GROUP_ARROWS = 0x37;
static const int MESH_LEFT = 0x81;
static const int MESH_LEFT_DISABLED = 0x83;
static const int SCOPE_UI = 2;

namespace UI {

void LeftArrowButton::render(midp::Graphics* g, int originY, int originX) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    int mesh = m_enabled ? MESH_LEFT : MESH_LEFT_DISABLED;
    quads.setGroupVisible(GROUP_ARROWS, true);
    quads.setMeshVisible(mesh, true);
    quads.setMeshBounds(mesh, (float)(m_x + originX), (float)(m_y + originY), (float)m_width,
                        (float)m_height, QuadManager::ANCHOR_LEFT | QuadManager::ANCHOR_TOP);
    quads.render(g, SCOPE_UI);
    quads.setMeshVisible(mesh, false);
    quads.setGroupVisible(GROUP_ARROWS, false);
}

LeftArrowButton::LeftArrowButton() {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    setWidth(quads.getMeshWidth(MESH_LEFT));
    setHeight(quads.getMeshHeight(MESH_LEFT));
}

LeftArrowButton::~LeftArrowButton() {}

}
