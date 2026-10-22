#include "game/Splines/TimedSpline.h"

#include "game/Splines.h"

Splines::TimedSpline::TimedSpline() : m_totalTime(0), m_numNodes(0) {}

void Splines::TimedSpline::clear() {
    m_numNodes = 0;
}

int Splines::TimedSpline::getNodeAtTime(int time) {
    int end = m_segmentTimes[0];
    int node = 0;
    while (end < time) {
        node++;
        if (node == m_numNodes) {
            node = 0;
        }
        end += m_segmentTimes[node];
    }
    return node;
}

MathVector Splines::TimedSpline::getPosition(int time) {
    int node = getNodeAtTime(time);
    int next = node + 1;
    if (next >= m_numNodes) {
        next = 0;
    }
    float t = m_invSegmentTimes[node] * (float)(time - m_nodeTimes[node]);
    MathVector m1 = m_tangents[next] * m_segmentLengths[next];
    MathVector p1(m_nodes[next]);
    MathVector p0(m_nodes[node]);
    MathVector m0 = m_tangents[node] * m_segmentLengths[node];
    return getPositionOnCubic(p0, m0, p1, m1, t);
}

MathVector Splines::TimedSpline::getNodePosition(int node) {
    return m_nodes[node];
}

void Splines::TimedSpline::buildSpline() {
    if (m_numNodes < 2) {
        return;
    }
    for (int i = 1; i < m_numNodes - 1; i++) {
        MathVector next = m_nodes[i + 1] - m_nodes[i];
        next.normalise();
        MathVector prev = m_nodes[i - 1] - m_nodes[i];
        prev.normalise();
        MathVector tangent = next - prev;
        tangent.normalise();
        m_tangents[i] = tangent;
    }
    int last = m_numNodes - 1;
    int beforeLast = m_numNodes - 2;
    MathVector dir = (m_nodes[1] - m_nodes[0]) * 3.0f / m_segmentLengths[0];
    m_tangents[0] = (dir - m_tangents[1]) * 0.5f;
    dir = (m_nodes[last] - m_nodes[beforeLast]) * 3.0f / m_segmentLengths[beforeLast];
    m_tangents[last] = (dir - m_tangents[beforeLast]) * 0.5f;
}

void Splines::TimedSpline::addNode(const MathVector& position, int time) {
    if (m_numNodes == 0) {
        m_totalTime = 0;
    } else {
        int last = m_numNodes - 1;
        m_segmentTimes[last] = time;
        m_segmentLengths[last] = (position - m_nodes[last]).getLength();
        if (time >= 1) {
            m_invSegmentTimes[last] = 1.0f / (float)time;
        } else {
            m_invSegmentTimes[last] = 0.0f;
        }
        m_totalTime += time;
    }
    m_nodes[m_numNodes] = position;
    m_nodeTimes[m_numNodes] = m_totalTime;
    m_numNodes++;
}

Splines::TimedSpline::~TimedSpline() {}
