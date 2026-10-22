#pragma once

#include "game/GameObject.h"

namespace particles {
class EmissionMode;
class ParticleEffect;
}

class GameObjectEffectHit : public GameObject {
public:
    static const int TYPE = 23;

    GameObjectEffectHit(MEdgeMap* map, m3g::Node* node);
    virtual ~GameObjectEffectHit();

    virtual void update(int dt);

    void updatePosition();

    midp::ReferenceCountedPointer<m3g::Node> m_node;
    midp::ReferenceCountedPointer<particles::EmissionMode> m_emission;
    midp::ReferenceCountedPointer<particles::ParticleEffect> m_effect;
    int m_timeLeft;
};
