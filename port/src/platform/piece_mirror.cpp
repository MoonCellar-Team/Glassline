#include "m3g/Appearance.h"
#include "m3g/Mesh.h"
#include "m3g/Node.h"
#include "m3g/PolygonMode.h"
#include "m3g/m3g.h"

namespace {

const int kWindingCcw = 168, kWindingCw = 169;

void flipWinding(m3g::Node* node) {
    if (m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node))
        for (int i = 0; i < mesh->getSubmeshCount(); i++) {
            m3g::Appearance* was = mesh->getAppearance(i);
            if (!was) continue;
            m3g::Appearance* look = static_cast<m3g::Appearance*>(was->duplicate());
            m3g::PolygonMode* mode = new m3g::PolygonMode();
            if (m3g::PolygonMode* old = was->getPolygonMode()) {
                mode->setCulling(old->getCulling());
                mode->setShading(old->getShading());
                mode->setTwoSidedLightingEnable(old->isTwoSidedLightingEnabled());
                mode->setLocalCameraLightingEnable(old->isLocalCameraLightingEnabled());
                mode->setPerspectiveCorrectionEnable(old->isPerspectiveCorrectionEnabled());
                mode->setWinding(old->getWinding() == kWindingCw ? kWindingCcw : kWindingCw);
            } else {
                mode->setWinding(kWindingCw);
            }
            look->setPolygonMode(mode);
            mesh->appearances().setElementAt(look, i);
        }
    if (m3g::Group* group = m3g::m3g_cast<m3g::Group>(node))
        for (int i = 0; i < group->getChildCount(); i++) flipWinding(group->getChild(i));
}

}

void medecomp_mirror_piece(m3g::Node* node) {
    if (!node) return;
    float scale[3];
    node->getScale(scale);
    node->setScale(-scale[0], scale[1], scale[2]);
    flipWinding(node);
}
