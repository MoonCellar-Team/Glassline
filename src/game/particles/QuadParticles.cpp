#include "game/particles/QuadParticles.h"

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

QuadParticles::QuadParticles(int maxParticles, ParticleMode* mode) : Particles(maxParticles, mode) {
}

int QuadParticles::getVertexCount() const {
    return getMaxParticleCount() * 4;
}

void QuadParticles::killParticle(int index, int firstVertex, m3g::VertexBuffer* vertices) {
    Particles::killParticle(index, firstVertex, vertices);
    static const float quadPosition[16] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    vertices->getPositions(NULL)->set(firstVertex + index * 4, 4, quadPosition);
}

void QuadParticles::updateParticle(int index, int firstVertex, m3g::VertexBuffer* vertices, float* position,
                                   float* velocity, float age, m3g::Transform* orientation,
                                   m3g::Transform* velocityTransform) {
    float life = getInverseLifetimeDuration(index) * age;

    float halfSize;
    m3g::KeyframeSequence* scale = getParticleMode()->getScale();
    if (scale == NULL) {
        halfSize = 0.5f;
    } else {
        halfSize = scale->sample((float)scale->getDuration() * life, 0) * 0.5f;
    }

    float angle;
    m3g::KeyframeSequence* rotation = getParticleMode()->getRotation();
    if (rotation == NULL) {
        angle = 0.0f;
    } else {
        angle = midp::Math::toRadians(rotation->sample((float)rotation->getDuration() * life, 0));
    }
    float s = midp::Math::sin(angle);
    float c = midp::Math::cos(angle);

    float up[4] = {s * halfSize, c * halfSize, 0.0f, 0.0f};
    float right[4] = {c * halfSize, -(s * halfSize), 0.0f, 0.0f};
    if (orientation != NULL) {
        orientation->transform(up, 4);
        orientation->transform(right, 4);
    }
    float corners[16];
    for (int k = 0; k < 4; k++) {
        corners[k] = position[k] + up[k] - right[k];
    }
    for (int k = 0; k < 4; k++) {
        corners[4 + k] = position[k] + up[k] + right[k];
    }
    for (int k = 0; k < 4; k++) {
        corners[8 + k] = position[k] - up[k] - right[k];
    }
    for (int k = 0; k < 4; k++) {
        corners[12 + k] = position[k] - up[k] + right[k];
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

m3g::IndexBuffer* QuadParticles::createIndexBuffer(int firstVertex) const {
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

QuadParticles::~QuadParticles() {
}

}
