#pragma once

#include "game/MathLine.h"
#include "game/MathPlane.h"

class MathFrustum {
public:
    class CullingPlane {
    public:
        CullingPlane();

        MathVector m_normal;
        float m_distance;
    };

    enum PlaneIndex {
        PLANE_NEAR = 0,
        PLANE_FAR = 1,
        PLANE_RIGHT = 2,
        PLANE_LEFT = 3,
        PLANE_TOP = 4,
        PLANE_BOTTOM = 5,
        NUM_PLANES = 6,
    };

    MathFrustum();
    ~MathFrustum();

    void set(const MathVector& from, const MathVector& at, float nearClip, float farClip, float fov,
             float width, float height);
    bool getScreenPositionFraction(const MathVector& pos, float& x, float& y) const;
    void getScreenPositionFractionNoZCheck(const MathVector& pos, float& x, float& y) const;
    const MathVector& getFocalPoint();
    int intersectAABB(const MathVector& min, const MathVector& max) const;
    int intersectAABBCoherency(const MathVector& min, const MathVector& max, unsigned int& coherency) const;
    int intersectXAABBCoherency(const MathVector& min, const MathVector& max, unsigned int& coherency) const;

    MathLine m_view;
    MathPlane m_farPlane;
    CullingPlane m_cullingPlanes[NUM_PLANES];
};
