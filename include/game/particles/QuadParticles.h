#pragma once

#include "game/particles/Particles.h"

namespace particles {

class QuadParticles : public Particles {
public:
    QuadParticles(int maxParticles, ParticleMode* mode);
    virtual ~QuadParticles();
    virtual int getVertexCount() const;
    virtual m3g::IndexBuffer* createIndexBuffer(int firstVertex) const;
    virtual void updateParticle(int index, int firstVertex, m3g::VertexBuffer* vertices, float* position,
                                float* velocity, float age, m3g::Transform* orientation,
                                m3g::Transform* velocityTransform);
    virtual void killParticle(int index, int firstVertex, m3g::VertexBuffer* vertices);
};

}
