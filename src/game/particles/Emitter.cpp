#include "game/particles/Emitter.h"

#include "game/particles/CylinderParticles.h"
#include "game/particles/EmissionMode.h"
#include "game/particles/Particles.h"
#include "game/particles/PointParticles.h"
#include "game/particles/QuadParticles.h"
#include "m3g/AnimationController.h"
#include "runtime/util/JRandom.h"

namespace particles {

EmissionMode* Emitter::getEmissionMode() const {
    return m_emissionMode;
}

Particles* Emitter::getParticles() const {
    return m_particles;
}

Emitter* Emitter::createEmitter(int type, int maxParticles, ParticleMode* mode, EmissionMode* emission) {
    midp::ReferenceCountedPointer<Particles> particles;
    if (type == PARTICLES_CYLINDER) {
        particles = new CylinderParticles(maxParticles, mode);
    } else if (type == PARTICLES_QUAD) {
        particles = new QuadParticles(maxParticles, mode);
    } else if (type == PARTICLES_POINT) {
        particles = new PointParticles(maxParticles, mode);
    } else {
        return NULL;
    }
    return new Emitter(particles, emission);
}

void Emitter::killAllParticles(int firstVertex, m3g::VertexBuffer* vertices) {
    m_particles->killAll(firstVertex, vertices);
}

void Emitter::update(int time, int firstVertex, m3g::VertexBuffer* vertices, m3g::Transform* orientation,
                     m3g::Transform* velocityTransform) {
    float position = m_animationController->getPosition(time);
    m_particles->update(position, firstVertex, vertices, orientation, velocityTransform, this);
}

int Emitter::getM3GUniqueClassID() const {
    return 127;
}

void Emitter::setAnimationController(m3g::AnimationController* controller) {
    m_animationController = controller;
}

void Emitter::setEmissionMode(EmissionMode* emission) {
    m_emissionMode = emission;
}

void Emitter::setRandomGenerator(util::JRandom* random) {
    m_random = random;
}

Emitter::Emitter(Particles* particles, EmissionMode* emission) {
    m_particles = particles;
    setRandomGenerator(new util::JRandom());
    setEmissionMode(emission);
    setAnimationController(new m3g::AnimationController());
}

Emitter::~Emitter() {
}

util::JRandom* Emitter::getRandomGenerator() const {
    return m_random;
}

}
