#pragma once

#include "m3g/Transformable.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace m3g {
class AnimationController;
class Transform;
class VertexBuffer;
}
namespace util {
class JRandom;
}

namespace particles {

class EmissionMode;
class ParticleMode;
class Particles;

class Emitter : public m3g::Transformable {
public:
    enum {
        PARTICLES_POINT = 0,
        PARTICLES_QUAD = 1,
        PARTICLES_CYLINDER = 2,
    };

    Emitter(Particles* particles, EmissionMode* emission);
    virtual ~Emitter();
    virtual int getM3GUniqueClassID() const;

    EmissionMode* getEmissionMode() const;
    Particles* getParticles() const;
    static Emitter* createEmitter(int type, int maxParticles, ParticleMode* mode, EmissionMode* emission);
    void killAllParticles(int firstVertex, m3g::VertexBuffer* vertices);
    void update(int time, int firstVertex, m3g::VertexBuffer* vertices, m3g::Transform* orientation,
                m3g::Transform* velocityTransform);
    void setAnimationController(m3g::AnimationController* controller);
    void setEmissionMode(EmissionMode* emission);
    void setRandomGenerator(util::JRandom* random);
    util::JRandom* getRandomGenerator() const;

    midp::ReferenceCountedPointer<Particles> m_particles;
    midp::ReferenceCountedPointer<m3g::AnimationController> m_animationController;
    midp::ReferenceCountedPointer<EmissionMode> m_emissionMode;
    midp::ReferenceCountedPointer<util::JRandom> m_random;
    unsigned char m_unkF4[12];
};

}
