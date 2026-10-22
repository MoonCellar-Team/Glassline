#include "game/particles/CylinderParticles.h"

#include "game/particles.h"
#include "game/particles/ParticleMode.h"
#include "m3g/IndexBuffer.h"
#include "m3g/KeyframeSequence.h"
#include "m3g/Transform.h"
#include "m3g/VertexBuffer.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/array.h"

namespace particles {

static const int INDEX_TRIANGLES = 8;

CylinderParticles::CylinderParticles(int maxParticles, ParticleMode* mode) : Particles(maxParticles, mode) {
}

int CylinderParticles::getVertexCount() const {
    return getMaxParticleCount() * 4;
}

void CylinderParticles::killParticle(int index, int firstVertex, m3g::VertexBuffer* vertices) {
    Particles::killParticle(index, firstVertex, vertices);
    static const float quadPosition[16] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    vertices->getPositions(NULL)->set(firstVertex + index * 4, 4, quadPosition);
}

void CylinderParticles::updateParticle(int index, int firstVertex, m3g::VertexBuffer* vertices, float* position,
                                       float* velocity, float age, m3g::Transform* orientation,
                                       m3g::Transform* velocityTransform) {
    float life = getInverseLifetimeDuration(index) * age;

    float halfWidth;
    float halfLength;
    m3g::KeyframeSequence* scale = getParticleMode()->getScale();
    if (scale == NULL) {
        halfWidth = 0.5f;
        halfLength = 0.5f;
    } else {
        float size[2];
        scale->sample((float)scale->getDuration() * life, 0, size);
        halfWidth = size[0] * 0.5f;
        halfLength = size[1] * 0.5f;
    }

    if (velocityTransform != NULL) {
        velocityTransform->transform(velocity, 4);
    }
    float along[4] = {-velocity[0], -velocity[1], velocity[2], 0.0f};
    float across[4] = {-velocity[1], velocity[0], 0.0f, 0.0f};
    float length =
        midp::Math::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
    if (length > 0.0f) {
        float inverse = 1.0f / length;
        along[0] *= inverse;
        along[1] *= inverse;
        along[2] *= inverse;
        along[3] = 0.0f;
    }
    float acrossLength = midp::Math::sqrt(across[0] * across[0] + across[1] * across[1]);
    if (acrossLength > 0.0f) {
        float inverse = 1.0f / acrossLength;
        across[0] *= inverse;
        across[1] *= inverse;
        across[2] = 0.0f;
        across[3] = 0.0f;
    }
    if (orientation != NULL) {
        orientation->transform(along, 4);
        orientation->transform(across, 4);
    }
    for (int k = 0; k < 3; k++) {
        along[k] *= halfLength;
    }
    for (int k = 0; k < 3; k++) {
        across[k] *= halfWidth;
    }

    float corners[16];
    for (int k = 0; k < 4; k++) {
        corners[k] = position[k] + along[k] - across[k];
        corners[4 + k] = position[k] + along[k] + across[k];
        corners[8 + k] = position[k] - along[k] - across[k];
        corners[12 + k] = position[k] - along[k] + across[k];
    }
    int vertex = firstVertex + index * 4;
    vertices->getPositions(NULL)->set(vertex, 4, corners);

    m3g::KeyframeSequence* color = getParticleMode()->getColor();
    if (color != NULL) {
        float rgba[4];
        color->sample((float)color->getDuration() * life, 0, rgba);
        signed char bytes[4];
        colorFloatsToBytes(rgba, bytes);
        signed char colors[16];
        for (int i = 0; i < 16; i++) {
            colors[i] = bytes[i & 3];
        }
        vertices->getColors()->set(vertex, 4, colors);
    }

    m3g::KeyframeSequence* crop = getParticleMode()->getCrop();
    if (crop != NULL) {
        float rect[4];
        crop->sample((float)crop->getDuration() * life, 0, rect);
        float texcoords[8] = {
            rect[0], rect[1],
            rect[0] + rect[2], rect[1],
            rect[0], rect[1] + rect[3],
            rect[0] + rect[2], rect[1] + rect[3],
        };
        vertices->getTexCoords(0, NULL)->set(vertex, 4, texcoords);
    }
}

m3g::IndexBuffer* CylinderParticles::createIndexBuffer(int firstVertex) const {
    int count = getMaxParticleCount();
    midp::array<int> indices(count * 6);
    int vertex = firstVertex;
    for (int i = 0; i < count; i++) {
        indices[i * 6 + 0] = vertex;
        indices[i * 6 + 1] = vertex + 2;
        indices[i * 6 + 2] = vertex + 1;
        indices[i * 6 + 3] = vertex + 1;
        indices[i * 6 + 4] = vertex + 2;
        indices[i * 6 + 5] = vertex + 3;
        vertex += 4;
    }
    return new m3g::IndexBuffer(INDEX_TRIANGLES, count * 2, indices);
}

CylinderParticles::~CylinderParticles() {
}

}
