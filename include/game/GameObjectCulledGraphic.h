#pragma once

#include "game/MathOrthoBox.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class MathFrustum;
namespace m3g {
class Node;
}

class GameObjectCulledGraphic : public midp::ReferenceCounted {
public:
    static const int SCREENS_UNKNOWN = 0xffffff;

    GameObjectCulledGraphic(m3g::Node* node, const MathOrthoBox& bounds);
    virtual ~GameObjectCulledGraphic();

    MathOrthoBox& getBounds();
    void setVisible(bool visible);
    bool isFullyCulled();
    void calcCull(int screen, const MathFrustum& frustum);
    void forceNoCull(int screen);

    midp::ReferenceCountedPointer<m3g::Node> m_node;
    MathOrthoBox m_bounds;
    int m_visibleScreens;
    bool m_visible;
};
