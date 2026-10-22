#pragma once

#include "m3g/Mesh.h"

namespace m3g {
class Transform;
}

namespace particles {

class Emitter;

class ParticleEffect : public m3g::Mesh {
public:
    explicit ParticleEffect(Emitter* emitter);
    virtual ~ParticleEffect();

    int getEmitterCount() const;
    Emitter* getEmitter(int index) const;
    void update(int time, m3g::Transform* orientation, m3g::Transform* velocityTransform);
    void updateStep(int dt, m3g::Transform* orientation, m3g::Transform* velocityTransform);
    void killAllParticles();
    void createVertexBuffer();
    void createIndexBuffers();
    void updateAppearances();

    int m_emitterCount;
    Emitter** m_emitters;
    int* m_firstVertices;
    int m_time;
    int m_unk13c;
};

}
