#include "game/ChunkManager.h"

#include "game/ChunkColumn.h"
#include "game/ChunkLayer.h"
#include "game/ChunkSection.h"
#include "runtime/midp/DataInputStream.h"

#ifdef MEDECOMP_PORT
void medecomp_chunk_load_begin();
#endif

void ChunkManager::update(float dt) {
    m_dt = dt;
}

m3g::Node* ChunkManager::getGraphicsReference(midp::DataInputStream& in, ChunkRunnerVision** runnerVision) {
    ChunkLayer* layer = m_layers[in.readShort()];
    ChunkColumn* column = layer->getColumn(in.readShort());
    ChunkSection* section = column->getSection(in.readShort());
    int node = in.readShort();
    if (runnerVision != NULL) {
        *runnerVision = section->getRunnerVisionChunk(node);
    }
    return section->getChunkNode(node);
}

ChunkManager::~ChunkManager() {}

ChunkManager::ChunkManager() : m_dt(0) {}

void ChunkManager::load(midp::DataInputStream& in, MapPalette& palette, m3g::Group* root) {
#ifdef MEDECOMP_PORT
    medecomp_chunk_load_begin();
#endif
    int count = in.readByte();
    m_layers = midp::array<midp::ReferenceCountedPointer<ChunkLayer> >(count);
    for (int i = 0; i != count; i++) {
        m_layers[i] = new ChunkLayer(in, palette, root);
    }
}

void ChunkManager::cullView(const MathFrustum& frustum) {
    int count = m_layers.m_length;
    for (int i = 0; i != count; i++) {
        m_layers[i]->updateViewCulling(m_dt, frustum);
    }
}
