#pragma once

#include "game/MathVector.h"

namespace Splines {

class TimedSpline {
public:
    static const int MAX_NODES = 50;

    TimedSpline();
    ~TimedSpline();
    void clear();
    int getNodeAtTime(int time);
    MathVector getPosition(int time);
    MathVector getNodePosition(int node);
    void buildSpline();
    void addNode(const MathVector& position, int time);

    MathVector m_nodes[MAX_NODES];
    MathVector m_tangents[MAX_NODES];
    int m_segmentTimes[MAX_NODES];
    float m_segmentLengths[MAX_NODES];
    float m_invSegmentTimes[MAX_NODES];
    int m_nodeTimes[MAX_NODES];
    int m_totalTime;
    int m_numNodes;
};

}
