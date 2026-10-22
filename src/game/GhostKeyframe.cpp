#include "game/GhostKeyframe.h"

GhostKeyframe::GhostKeyframe() : m_deltaTime(0), m_visual(-1), m_blendWeight(0.0f) {}

GhostKeyframe::GhostKeyframe(int deltaTime, const MathVector& position, int visual, float blendWeight)
    : m_deltaTime(deltaTime), m_position(position), m_visual((short)visual), m_blendWeight(blendWeight) {}

GhostKeyframe::GhostKeyframe(const GhostKeyframe& other)
    : midp::ReferenceCounted(),
      m_deltaTime(other.m_deltaTime),
      m_position(other.m_position),
      m_visual(other.m_visual),
      m_blendWeight(other.m_blendWeight) {}

GhostKeyframe::~GhostKeyframe() {}
