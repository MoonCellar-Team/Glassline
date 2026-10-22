#include "game/particles/ParticleEffect.h"

#include "game/particles/Emitter.h"
#include "game/particles/ParticleMode.h"
#include "game/particles/Particles.h"
#include "m3g/IndexBuffer.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"

namespace particles {

static const int COMPONENT_BYTE = 1;
static const int COMPONENT_FLOAT = 4;

int ParticleEffect::getEmitterCount() const {
    return m_emitterCount;
}

Emitter* ParticleEffect::getEmitter(int index) const {
    return m_emitters[index];
}

void ParticleEffect::update(int time, m3g::Transform* orientation, m3g::Transform* velocityTransform) {
    int count = getEmitterCount();
    for (int i = 0; i < count; i++) {
        Emitter* emitter = getEmitter(i);
        emitter->update(time, m_firstVertices[i], getVertexBuffer(), orientation, velocityTransform);
    }
    m_time = time;
}

void ParticleEffect::updateStep(int dt, m3g::Transform* orientation, m3g::Transform* velocityTransform) {
    update(m_time + dt, orientation, velocityTransform);
}

ParticleEffect::~ParticleEffect() {
    for (int i = 0; i < m_emitterCount; i++) {
        midp::DECREF(m_emitters[i]);
    }
    if (m_emitters != NULL) {
        delete[] m_emitters;
    }
    if (m_firstVertices != NULL) {
        delete[] m_firstVertices;
    }
}

void ParticleEffect::killAllParticles() {
    int count = getEmitterCount();
    for (int i = 0; i < count; i++) {
        Emitter* emitter = getEmitter(i);
        emitter->killAllParticles(m_firstVertices[i], getVertexBuffer());
    }
}

void ParticleEffect::createVertexBuffer() {
    int count = getEmitterCount();
    int vertexCount = 0;
    bool colors = false;
    bool normals = false;
    bool texcoords = false;
    bool pointSizes = false;
    for (int i = 0; i < count; i++) {
        Particles* particles = getEmitter(i)->getParticles();
        m_firstVertices[i] = vertexCount;
        vertexCount += particles->getVertexCount();
        colors |= particles->hasColors();
        normals |= particles->hasNormals();
        texcoords |= particles->hasTexcoords();
        pointSizes |= particles->hasPointSizes();
    }

    m3g::VertexBuffer* vertices = new m3g::VertexBuffer();
    vertices->setPositions(new m3g::VertexArray(vertexCount, 4, COMPONENT_FLOAT), 1.0f, NULL);
    if (colors) {
        vertices->setColors(new m3g::VertexArray(vertexCount, 4, COMPONENT_BYTE));
    }
    if (normals) {
        vertices->setNormals(new m3g::VertexArray(vertexCount, 3, COMPONENT_FLOAT));
    }
    if (texcoords) {
        vertices->setTexCoords(0, new m3g::VertexArray(vertexCount, 2, COMPONENT_FLOAT), 1.0f, NULL);
    }
    if (pointSizes) {
        vertices->setPointSizes(new m3g::VertexArray(vertexCount, 1, COMPONENT_FLOAT));
    }
    setVertexBuffer(vertices);
    killAllParticles();
}

void ParticleEffect::createIndexBuffers() {
    int count = getEmitterCount();
    for (int i = 0; i < count; i++) {
        Particles* particles = getEmitter(i)->getParticles();
        setIndexBuffer(i, particles->createIndexBuffer(m_firstVertices[i]));
    }
}

void ParticleEffect::updateAppearances() {
    int count = getEmitterCount();
    for (int i = 0; i < count; i++) {
        setAppearanceBase(i, getEmitter(i)->getParticles()->getParticleMode()->getAppearance());
    }
}

ParticleEffect::ParticleEffect(Emitter* emitter)
    : m3g::Mesh(1, 0), m_emitterCount(1), m_emitters(NULL), m_firstVertices(NULL), m_time(0) {
    m_emitters = new Emitter*[m_emitterCount];
    midp::INCREF(emitter);
    m_emitters[0] = emitter;
    m_firstVertices = new int[1];
    createVertexBuffer();
    createIndexBuffers();
    updateAppearances();
}

}
