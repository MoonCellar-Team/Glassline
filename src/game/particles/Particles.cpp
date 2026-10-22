#include "game/particles/Particles.h"

#include "game/particles/EmissionMode.h"
#include "game/particles/Emitter.h"
#include "game/particles/ParticleMode.h"
#include "m3g/Transform.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/System.h"
#include "runtime/util/JRandom.h"

namespace particles {

ParticleMode* Particles::getParticleMode() const {
    return m_particleMode;
}

void Particles::generateParticle(int index, Emitter* emitter, m3g::Transform* transform) {
    util::JRandom* random = emitter->getRandomGenerator();
    setLifetimeDuration(index, generateLifetimeDuration(random));
    EmissionMode* emission = emitter->getEmissionMode();
    float position[4];
    float velocity[4];
    emission->getParticleStartPosition(random, position);
    emission->getParticleStartVelocity(random, velocity);
    if (transform != NULL) {
        transform->transform(position, 4);
        transform->transform(velocity, 4);
    }
    midp::System::arraycopy(position, 0, m_startPositions, index * 4, 4);
    midp::System::arraycopy(velocity, 0, m_startVelocities, index * 4, 4);
    m_alive[index] = true;
}

void Particles::killParticle(int index, int firstVertex, m3g::VertexBuffer* vertices) {
    m_alive[index] = false;
}

void Particles::killAll(int firstVertex, m3g::VertexBuffer* vertices) {
    int count = getMaxParticleCount();
    for (int i = 0; i < count; i++) {
        killParticle(i, firstVertex, vertices);
    }
}

void Particles::update(float time, int firstVertex, m3g::VertexBuffer* vertices, m3g::Transform* orientation,
                       m3g::Transform* velocityTransform, Emitter* emitter) {
    m3g::Transform toEffect;
    emitter->getCompositeTransform(&toEffect);
    EmissionMode* emission = emitter->getEmissionMode();
    util::JRandom* random = emitter->getRandomGenerator();
    int count = getMaxParticleCount();

    float emitTime;
    float interval;
    float rate = emission->getRate();
    if (rate > 0.0f) {
        interval = 1000.0f / rate;
        emitTime = midp::Math::abs(time - m_lastEmitTime);
        if (emitTime >= interval) {
            m_lastEmitTime = time;
        }
    } else {
        emitTime = 0.0f;
        interval = 1000.0f;
    }

    const float* acceleration = emission->getAcceleration();
    for (int i = 0; i < count; i++) {
        float lifetime = m_lifetimes[i];
        float startTime = m_startTimes[i];
        bool alive = isParticleAlive(i);
        float age = time - startTime;
        bool beforeBirth = alive && age < 0.0f;
        if (!alive || beforeBirth || age > lifetime) {
            if (emitTime < interval) {
                if (alive) {
                    killParticle(i, firstVertex, vertices);
                }
                continue;
            }
            generateParticle(i, emitter, &toEffect);
            emitTime -= interval;
            if (beforeBirth) {
                float duration = getLifetimeDuration(i);
                startTime = generateStartTime(time, random) - duration;
            } else {
                startTime = generateStartTime(time, random);
            }
            age = time - startTime;
            m_startTimes[i] = startTime;
        }

        float seconds = age * 0.001f;
        float position[4];
        float velocity[4];
        for (int k = 0; k < 4; k++) {
            position[k] = m_startPositions[i * 4 + k] + m_startVelocities[i * 4 + k] * seconds;
            velocity[k] = m_startVelocities[i * 4 + k];
        }
        if (acceleration != NULL) {
            float halfSquare = seconds * seconds * 0.5f;
            for (int k = 0; k < 3; k++) {
                position[k] += acceleration[k] * halfSquare;
                velocity[k] += acceleration[k] * seconds;
            }
        }
        updateParticle(i, firstVertex, vertices, position, velocity, age, orientation, velocityTransform);
    }
    m_lastTime = time;
}

bool Particles::isParticleAlive(int index) const {
    return m_alive[index];
}

bool Particles::hasNormals() const {
    return false;
}

bool Particles::hasPointSizes() const {
    return false;
}

float Particles::getLifetimeDuration(int index) const {
    return m_lifetimes[index];
}

int Particles::getMaxParticleCount() const {
    return m_maxParticles;
}

void Particles::setLifetimeDuration(int index, float duration) {
    m_lifetimes[index] = duration;
    m_inverseLifetimes[index] = 1.0f / duration;
}

bool Particles::hasTexcoords() const {
    return getParticleMode()->getCrop() != NULL;
}

bool Particles::hasColors() const {
    return getParticleMode()->getColor() != NULL;
}

Particles::~Particles() {
    if (m_alive != NULL) {
        delete[] m_alive;
    }
    if (m_startTimes != NULL) {
        delete[] m_startTimes;
    }
    if (m_lifetimes != NULL) {
        delete[] m_lifetimes;
    }
    if (m_inverseLifetimes != NULL) {
        delete[] m_inverseLifetimes;
    }
    if (m_startPositions != NULL) {
        delete[] m_startPositions;
    }
    if (m_startVelocities != NULL) {
        delete[] m_startVelocities;
    }
}

float Particles::generateStartTime(float time, util::JRandom* random) {
    float r = random->nextFloat();
    return (1.0f - r) * time + m_lastTime * r;
}

float Particles::generateLifetimeDuration(util::JRandom* random) {
    ParticleMode* mode = getParticleMode();
    float mean = mode->getMeanTimeToLive();
    float deviation = mode->getMeanTimeToLiveDeviation();
    float r = random->nextFloat();
    return mean + (r + r + -1.0f) * deviation;
}

Particles::Particles(int maxParticles, ParticleMode* mode)
    : m_maxParticles(maxParticles), m_alive(NULL), m_lifetimes(NULL), m_inverseLifetimes(NULL),
      m_startTimes(NULL), m_lastEmitTime(0.0f), m_lastTime(0.0f), m_startPositions(NULL),
      m_startVelocities(NULL) {
    m_particleMode = mode;
    m_alive = new bool[maxParticles];
    for (int i = 0; i < maxParticles; i++) {
        m_alive[i] = false;
    }
    m_startTimes = new float[maxParticles];
    m_lifetimes = new float[maxParticles];
    m_inverseLifetimes = new float[maxParticles];
    m_startPositions = new float[maxParticles * 4];
    m_startVelocities = new float[maxParticles * 4];
}

float Particles::getInverseLifetimeDuration(int index) const {
    return m_inverseLifetimes[index];
}

}
