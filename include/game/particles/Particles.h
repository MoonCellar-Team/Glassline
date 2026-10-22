#pragma once

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace m3g {
class IndexBuffer;
class Transform;
class VertexBuffer;
}
namespace util {
class JRandom;
}

namespace particles {

class Emitter;
class ParticleMode;

class Particles : public midp::ReferenceCounted {
public:
    Particles(int maxParticles, ParticleMode* mode);
    virtual ~Particles();
    virtual int getVertexCount() const = 0;
    virtual m3g::IndexBuffer* createIndexBuffer(int firstVertex) const = 0;
    virtual bool hasColors() const;
    virtual bool hasNormals() const;
    virtual bool hasTexcoords() const;
    virtual bool hasPointSizes() const;
    virtual void updateParticle(int index, int firstVertex, m3g::VertexBuffer* vertices, float* position,
                                float* velocity, float age, m3g::Transform* orientation,
                                m3g::Transform* velocityTransform) = 0;
    virtual void killParticle(int index, int firstVertex, m3g::VertexBuffer* vertices);

    ParticleMode* getParticleMode() const;
    void generateParticle(int index, Emitter* emitter, m3g::Transform* transform);
    void killAll(int firstVertex, m3g::VertexBuffer* vertices);
    void update(float time, int firstVertex, m3g::VertexBuffer* vertices, m3g::Transform* orientation,
                m3g::Transform* velocityTransform, Emitter* emitter);
    bool isParticleAlive(int index) const;
    float getLifetimeDuration(int index) const;
    int getMaxParticleCount() const;
    void setLifetimeDuration(int index, float duration);
    float generateStartTime(float time, util::JRandom* random);
    float generateLifetimeDuration(util::JRandom* random);
    float getInverseLifetimeDuration(int index) const;

    int m_maxParticles;
    midp::ReferenceCountedPointer<ParticleMode> m_particleMode;
    bool* m_alive;
    float* m_lifetimes;
    float* m_inverseLifetimes;
    float* m_startTimes;
    float m_lastEmitTime;
    float m_lastTime;
    float* m_startPositions;
    float* m_startVelocities;
};

}
