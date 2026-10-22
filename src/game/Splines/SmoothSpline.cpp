#include "game/Splines/SmoothSpline.h"

#include "game/Splines.h"

int Splines::SmoothSpline::getNodeAtLength(float length) const {
    float end = m_segmentLengths[0];
    int node = 0;
    while (end < length) {
        node++;
        if (node == m_numNodes) {
            node = 0;
        }
        end = m_segmentLengths[node] + end;
    }
    return node;
}

MathVector Splines::SmoothSpline::getPosition(float length) const {
    int node = getNodeAtLength(length);
    int next = node + 1;
    if (next >= m_numNodes) {
        next = 0;
    }
    float t = m_invSegmentLengths[node] * (length - m_nodeLengths[node]);
    MathVector m1 = m_tangents[next] * m_segmentLengths[next];
    MathVector p1(m_nodes[next]);
    MathVector p0(m_nodes[node]);
    MathVector m0 = m_tangents[node] * m_segmentLengths[node];
    return getPositionOnCubic(p0, m0, p1, m1, t);
}

void Splines::SmoothSpline::buildSpline() {
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

void Splines::SmoothSpline::addNode(const MathVector& position) {
    if (m_numNodes == 0) {
        m_totalLength = 0.0f;
    } else {
        int last = m_numNodes - 1;
        float length = (m_nodes[last] - position).getLength();
        m_segmentLengths[last] = length;
        if (length > 0.0f) {
            m_invSegmentLengths[last] = 1.0f / length;
        } else {
            m_invSegmentLengths[last] = 0.0f;
        }
        m_totalLength += length;
    }
    m_nodes[m_numNodes] = position;
    m_nodeLengths[m_numNodes] = m_totalLength;
    m_numNodes++;
}

Splines::SmoothSpline::SmoothSpline() : m_totalLength(0.0f), m_numNodes(0) {}

float Splines::SmoothSpline::getMaxLength() const {
    return m_totalLength;
}

Splines::SmoothSpline::~SmoothSpline() {}
