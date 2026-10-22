#include "game/MathFrustum.h"

#include "runtime/midp/Math.h"

MathFrustum::MathFrustum() {}

MathFrustum::~MathFrustum() {}

int MathFrustum::intersectAABB(const MathVector& min, const MathVector& max) const {
    int result = 1;
    for (int i = PLANE_RIGHT; i < NUM_PLANES; i++) {
        const CullingPlane& plane = m_cullingPlanes[i];
        const MathVector& n = plane.m_normal;
        MathVector farCorner(n.x > 0.0f ? max.x : min.x, n.y > 0.0f ? max.y : min.y, n.z > 0.0f ? max.z : min.z);
        MathVector nearCorner(n.x < 0.0f ? max.x : min.x, n.y < 0.0f ? max.y : min.y,
                              n.z < 0.0f ? max.z : min.z);
        if (n.dot(nearCorner) + plane.m_distance > 0.0f) {
            return -1;
        }
        if (n.dot(farCorner) + plane.m_distance >= 0.0f) {
            result = 0;
        }
    }
    return result;
}

int MathFrustum::intersectAABBCoherency(const MathVector& min, const MathVector& max,
                                        unsigned int& coherency) const {
    for (unsigned int i = 1; i < NUM_PLANES; i++) {
        if (i == coherency) {
            continue;
        }
        unsigned int index = i;
        if (i == 1 && coherency <= 5) {
            index = coherency;
        }
        const CullingPlane& plane = m_cullingPlanes[index];
        const MathVector& n = plane.m_normal;
        MathVector nearCorner(n.x < 0.0f ? max.x : min.x, n.y < 0.0f ? max.y : min.y,
                              n.z < 0.0f ? max.z : min.z);
        if (n.dot(nearCorner) + plane.m_distance > 0.05f) {
            coherency = index;
            return -1;
        }
    }
    return 1;
}

int MathFrustum::intersectXAABBCoherency(const MathVector& min, const MathVector& max,
                                         unsigned int& coherency) const {
    for (unsigned int i = 1; i < PLANE_TOP; i++) {
        if (i == coherency) {
            continue;
        }
        unsigned int index = i;
        if (i == 1 && coherency - 2 <= 1) {
            index = coherency;
        }
        const CullingPlane& plane = m_cullingPlanes[index];
        const MathVector& n = plane.m_normal;
        MathVector nearCorner(n.x < 0.0f ? max.x : min.x, n.y < 0.0f ? max.y : min.y,
                              n.z < 0.0f ? max.z : min.z);
        if (n.dot(nearCorner) + plane.m_distance > 0.05f) {
            coherency = index;
            return -1;
        }
    }
    return 1;
}

bool MathFrustum::getScreenPositionFraction(const MathVector& pos, float& x, float& y) const {
    float depth = m_view.calculateClosestTToPoint(pos);
    if (depth < 0.0 || depth > 1.0) {
        return false;
    }
    MathLine axis(m_farPlane.m_center, m_farPlane.m_up);
    axis.m_direction *= depth;
    y = axis.calculateClosestTToPoint(pos) * -0.5f + 0.5f;
    axis.m_direction = m_farPlane.m_right;
    axis.m_direction *= depth;
    x = (axis.calculateClosestTToPoint(pos) + 1.0f) * 0.5f;
    return true;
}

void MathFrustum::getScreenPositionFractionNoZCheck(const MathVector& pos, float& x, float& y) const {
    float depth = m_view.calculateClosestTToPoint(pos);
    if (depth == 0.0) {
        MathLine axis(m_farPlane.m_center, m_farPlane.m_up);
        float v = axis.calculateClosestTToPoint(pos);
        axis.m_direction = m_farPlane.m_right;
        float h = axis.calculateClosestTToPoint(pos);
        float scale = 3.0f / midp::Math::sqrt(h * h + v * v);
        x = h * scale;
        y = -(v * scale);
    } else {
        MathLine axis(m_farPlane.m_center, m_farPlane.m_up);
        axis.m_direction *= depth;
        y = axis.calculateClosestTToPoint(pos) * -0.5f + 0.5f;
        axis.m_direction = m_farPlane.m_right;
        axis.m_direction *= depth;
        x = (axis.calculateClosestTToPoint(pos) + 1.0f) * 0.5f;
        if (depth < 0.0) {
            x = 1.0f - x;
            y = 1.0f - y;
        }
    }
}

void MathFrustum::set(const MathVector& from, const MathVector& at, float nearClip, float farClip, float fov,
                      float width, float height) {
    float aspect = height / width;
    double fovRad = midp::Math::toRadians(fov);
    float tanV = midp::Math::tan(fovRad * aspect * 0.5) * 1.1f;
    float tanH = midp::Math::tan(fovRad * 0.5);
    MathVector dir(at.x - from.x, at.y - from.y, at.z - from.z);
    dir.normalise();

    m_view.m_origin = from;
    m_view.m_direction.set(dir);
    m_view.m_direction.setLength(farClip);
    m_farPlane.m_center.set(from.x + m_view.m_direction.x, from.y + m_view.m_direction.y,
                            from.z + m_view.m_direction.z);
    m_farPlane.m_up.set(0.0f, 1.0f, 0.0f);
    m_view.m_direction.cross(m_farPlane.m_up, m_farPlane.m_right);
    m_farPlane.m_right.cross(m_view.m_direction, m_farPlane.m_up);
    m_farPlane.m_up.setLength(tanV * farClip);
    m_farPlane.m_right.setLength(tanH * farClip);

    MathVector nearCenter = from + dir * nearClip;
    MathVector farCenter = from + dir * farClip;
    MathVector right(m_farPlane.m_right);
    MathVector up(m_farPlane.m_up);
    right.normalise();
    up.normalise();
    MathVector nearTopRight = nearCenter + up * tanV * nearClip + right * tanH * nearClip;
    MathVector nearTopLeft = nearCenter + up * tanV * nearClip - right * tanH * nearClip;
    MathVector nearBottomRight = nearCenter - up * tanV * nearClip + right * tanH * nearClip;
    MathVector nearBottomLeft = nearCenter - up * tanV * nearClip - right * tanH * nearClip;
    MathVector farTopRight = farCenter + up * tanV * farClip + right * tanH * farClip;
    MathVector farTopLeft = farCenter + up * tanV * farClip - right * tanH * farClip;
    MathVector farBottomRight = farCenter - up * tanV * farClip + right * tanH * farClip;
    MathVector farBottomLeft = farCenter - up * tanV * farClip - right * tanH * farClip;

    CullingPlane& rightPlane = m_cullingPlanes[PLANE_RIGHT];
    rightPlane.m_normal = (nearBottomRight - nearTopRight).cross(farBottomRight - nearTopRight);
    rightPlane.m_normal.normalise();
    rightPlane.m_distance = -nearTopRight.dot(rightPlane.m_normal);

    CullingPlane& leftPlane = m_cullingPlanes[PLANE_LEFT];
    leftPlane.m_normal = (nearTopLeft - nearBottomLeft).cross(farBottomLeft - nearBottomLeft);
    leftPlane.m_normal.normalise();
    leftPlane.m_distance = -nearBottomLeft.dot(leftPlane.m_normal);

    CullingPlane& topPlane = m_cullingPlanes[PLANE_TOP];
    topPlane.m_normal = (nearTopRight - nearTopLeft).cross(farTopRight - nearTopLeft);
    topPlane.m_normal.normalise();
    topPlane.m_distance = -nearTopLeft.dot(topPlane.m_normal);

    CullingPlane& bottomPlane = m_cullingPlanes[PLANE_BOTTOM];
    bottomPlane.m_normal = (nearBottomLeft - nearBottomRight).cross(farBottomLeft - nearBottomRight);
    bottomPlane.m_normal.normalise();
    bottomPlane.m_distance = -nearBottomRight.dot(bottomPlane.m_normal);
}

const MathVector& MathFrustum::getFocalPoint() {
    return m_view.m_origin;
}
