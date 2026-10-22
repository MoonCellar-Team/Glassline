#pragma once

#include "game/MathOrthoBox.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class MathVector;
namespace m3g {
class Node;
}
namespace midp {
class DataInputStream;
}

class ChunkRunnerVision : public midp::ReferenceCounted {
public:
    enum Type {
        TYPE_LEFT = 1,
        TYPE_RIGHT = 2,
        TYPE_BOTH = 3,
    };

    ChunkRunnerVision(midp::DataInputStream& in, m3g::Node* node);
    virtual ~ChunkRunnerVision();

    int getType();
    void updateIntensity(float dt, const MathVector& position, int facing);

    int m_type;
    MathOrthoBox m_bounds;
    midp::ReferenceCountedPointer<m3g::Node> m_node;
    int m_color;
    int m_targetColor;
};
