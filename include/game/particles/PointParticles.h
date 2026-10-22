#pragma once

#include "game/particles/Particles.h"

namespace particles {

class PointParticles : public Particles {
public:
    PointParticles(int maxParticles, ParticleMode* mode);
    virtual ~PointParticles();
    virtual int getVertexCount() const;
    virtual m3g::IndexBuffer* createIndexBuffer(int firstVertex) const;
    virtual bool hasTexcoords() const;
    virtual bool hasPointSizes() const;
    virtual void updateParticle(int index, int firstVertex, m3g::VertexBuffer* vertices, float* position,
                                float* velocity, float age, m3g::Transform* orientation,
                                m3g::Transform* velocityTransform);
    virtual void killParticle(int index, int firstVertex, m3g::VertexBuffer* vertices);

    static void colorFloatsToBytes(float* rgba, signed char* out);
};

}
