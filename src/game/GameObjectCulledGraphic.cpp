#include "game/GameObjectCulledGraphic.h"

#include "game/M3GAssets.h"
#include "game/MathFrustum.h"
#include "m3g/Node.h"

static const int AABB_OUTSIDE = -1;

MathOrthoBox& GameObjectCulledGraphic::getBounds() {
    return m_bounds;
}

void GameObjectCulledGraphic::setVisible(bool visible) {
    m_visible = visible;
}

bool GameObjectCulledGraphic::isFullyCulled() {
    return m_visibleScreens == 0;
}

GameObjectCulledGraphic::GameObjectCulledGraphic(m3g::Node* node, const MathOrthoBox& bounds)
    : m_node(node), m_bounds(bounds) {
    m_visibleScreens = SCREENS_UNKNOWN;
    m_visible = node->isRenderingEnabled();
}

void GameObjectCulledGraphic::calcCull(int screen, const MathFrustum& frustum) {
    if (m_visibleScreens == SCREENS_UNKNOWN) {
        m_visibleScreens = 0;
    }
    int result = frustum.intersectAABB(m_bounds.m_min, m_bounds.m_max);
    m_node->setRenderingEnable(m_visible && result != AABB_OUTSIDE);
    if (result == AABB_OUTSIDE) {
        m_visibleScreens &= ~(1 << screen);
    } else {
        m_visibleScreens |= 1 << screen;
    }
}

void GameObjectCulledGraphic::forceNoCull(int screen) {
    m_visibleScreens |= 1 << screen;
    m_node->setRenderingEnable(m_visible);
}

GameObjectCulledGraphic::~GameObjectCulledGraphic() {
    M3GAssets::orphanNode(m_node);
}
