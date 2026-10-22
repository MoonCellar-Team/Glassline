#pragma once

#include "game/MathVector.h"
#include "runtime/midp/ReferenceCounted.h"

class GhostKeyframe : public midp::ReferenceCounted {
public:
    GhostKeyframe();
    GhostKeyframe(int deltaTime, const MathVector& position, int visual, float blendWeight);
    GhostKeyframe(const GhostKeyframe& other);
    virtual ~GhostKeyframe();

    int m_deltaTime;
    MathVector m_position;
    short m_visual;
    float m_blendWeight;
};
