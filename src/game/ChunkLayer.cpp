#include "game/ChunkLayer.h"

#include "game/AppEngine.h"
#include "game/ChunkColumn.h"
#include "game/M3GAssets.h"
#include "game/MathFrustum.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"
#include "runtime/midp/DataInputStream.h"

static const int FRUSTUM_OUTSIDE = -1;

ChunkLayer::~ChunkLayer() {}

ChunkLayer::ChunkLayer(midp::DataInputStream& in, MapPalette& palette, m3g::Group* root)
    : m_unk1c(0), m_columnCount(0), m_unk24(0) {
    int count = in.readByte();
    m_columns = midp::array<midp::ReferenceCountedPointer<ChunkColumn> >(count);
    m_columnCount = count;
    for (int i = 0; i != count; i++) {
        ChunkColumn* column = new ChunkColumn(in, palette);
        m_columns[i] = column;
        M3GAssets::addNode(root, column->getColumnGroup());
    }
}

void ChunkLayer::updateViewCulling(float dt, const MathFrustum& frustum) {
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    int count = m_columns.m_length;
    for (int i = 0; i != count; i++) {
        MathOrthoBox& bounds = m_columns[i]->getBounds();
        if (frustum.intersectXAABBCoherency(bounds.m_min, bounds.m_max, m_columns[i]->getCoherencyIndex()) ==
            FRUSTUM_OUTSIDE) {
            m_columns[i]->getColumnGroup()->setRenderingEnable(false);
        } else {
            m_columns[i]->update(dt, frustum, player);
        }
    }
}

ChunkColumn* ChunkLayer::getColumn(int index) {
    return m_columns[index];
}
