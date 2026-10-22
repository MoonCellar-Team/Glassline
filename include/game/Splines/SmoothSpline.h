#pragma once

#include "game/MathVector.h"

namespace Splines {

class SmoothSpline {
public:
    static const int MAX_NODES = 50;

    SmoothSpline();
    ~SmoothSpline();
    int getNodeAtLength(float length) const;
    MathVector getPosition(float length) const;
    void buildSpline();
    void addNode(const MathVector& position);
    float getMaxLength() const;

    MathVector m_nodes[MAX_NODES];
    MathVector m_tangents[MAX_NODES];
    float m_segmentLengths[MAX_NODES];
    float m_invSegmentLengths[MAX_NODES];
    float m_nodeLengths[MAX_NODES];
    float m_totalLength;
    int m_numNodes;
};

}
