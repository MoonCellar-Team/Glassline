#pragma once

#include "game/MathOrthoBox.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class ChunkRunnerVision;
class GameObjectPlayer;
class MapPalette;
namespace m3g {
class Node;
}
namespace midp {
class DataInputStream;
}

class ChunkSection : public midp::ReferenceCounted {
public:
    ChunkSection(midp::DataInputStream& in, MapPalette& palette);
    virtual ~ChunkSection();

    m3g::Node* getChunkNode(int index);
    m3g::Node* readNode(midp::DataInputStream& in, MapPalette& palette);
    void updateRunnerVision(float dt, GameObjectPlayer* player);
    MathOrthoBox& getBounds();
    unsigned int& getCoherencyIndex();
    m3g::Node* getSectionNode();
    ChunkRunnerVision* getRunnerVisionChunk(int index);

    MathOrthoBox m_bounds;
    int m_nodeCount;
    midp::ReferenceCountedPointer<m3g::Node> m_node;
    midp::array<midp::ReferenceCountedPointer<ChunkRunnerVision> > m_runnerVision;
    unsigned int m_coherencyIndex;
};
