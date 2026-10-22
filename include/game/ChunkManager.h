#pragma once

#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class ChunkLayer;
class ChunkRunnerVision;
class MapPalette;
class MathFrustum;
namespace m3g {
class Group;
class Node;
}
namespace midp {
class DataInputStream;
}

class ChunkManager {
public:
    ChunkManager();
    ~ChunkManager();

    void update(float dt);
    void load(midp::DataInputStream& in, MapPalette& palette, m3g::Group* root);
    void cullView(const MathFrustum& frustum);
    m3g::Node* getGraphicsReference(midp::DataInputStream& in, ChunkRunnerVision** runnerVision);

    float m_dt;
    midp::array<midp::ReferenceCountedPointer<ChunkLayer> > m_layers;
};
