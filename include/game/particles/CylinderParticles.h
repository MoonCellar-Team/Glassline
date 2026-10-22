#pragma once

#include "game/particles/Particles.h"

namespace particles {

class CylinderParticles : public Particles {
public:
    CylinderParticles(int maxParticles, ParticleMode* mode);
    virtual ~CylinderParticles();
    virtual int getVertexCount() const;
    virtual m3g::IndexBuffer* createIndexBuffer(int firstVertex) const;
    virtual void updateParticle(int index, int firstVertex, m3g::VertexBuffer* vertices, float* position,
                                float* velocity, float age, m3g::Transform* orientation,
                                m3g::Transform* velocityTransform);
    virtual void killParticle(int index, int firstVertex, m3g::VertexBuffer* vertices);
};

}
