#include "game/particles/PointParticles.h"

#include "game/particles.h"
#include "game/particles/ParticleMode.h"
#include "m3g/IndexBuffer.h"
#include "m3g/KeyframeSequence.h"
#include "m3g/VertexBuffer.h"

namespace particles {

static const int INDEX_POINT_SPRITES = 10;

PointParticles::PointParticles(int maxParticles, ParticleMode* mode) : Particles(maxParticles, mode) {
}

int PointParticles::getVertexCount() const {
    return getMaxParticleCount();
}

bool PointParticles::hasTexcoords() const {
    return false;
}

bool PointParticles::hasPointSizes() const {
    return true;
}

void PointParticles::killParticle(int index, int firstVertex, m3g::VertexBuffer* vertices) {
    Particles::killParticle(index, firstVertex, vertices);
    float size = 0.0f;
    vertices->getPointSizes()->set(firstVertex + index, 1, &size);
}

void PointParticles::updateParticle(int index, int firstVertex, m3g::VertexBuffer* vertices, float* position,
                                    float* velocity, float age, m3g::Transform* orientation,
                                    m3g::Transform* velocityTransform) {
    int vertex = firstVertex + index;
    vertices->getPositions(NULL)->set(vertex, 1, position);
    m3g::KeyframeSequence* color = getParticleMode()->getColor();
    if (color != NULL) {
        float rgba[4];
        color->sample(age, 0, rgba);
        signed char bytes[4];
        colorFloatsToBytes(rgba, bytes);
        vertices->getColors()->set(vertex, 1, bytes);
    }
}

m3g::IndexBuffer* PointParticles::createIndexBuffer(int firstVertex) const {
    return new m3g::IndexBuffer(INDEX_POINT_SPRITES, getVertexCount(), firstVertex);
}

PointParticles::~PointParticles() {
}

void PointParticles::colorFloatsToBytes(float* rgba, signed char* out) {
    particles::colorFloatsToBytes(rgba, out);
}

}
