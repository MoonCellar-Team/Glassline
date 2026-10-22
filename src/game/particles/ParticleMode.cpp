#include "game/particles/ParticleMode.h"

#include "m3g/AppearanceBase.h"
#include "m3g/KeyframeSequence.h"
#include "runtime/midp/String.h"

namespace particles {

midp::Class* ParticleMode::getClass() const {
    return NULL;
}

void ParticleMode::setMeanTimeToLive(float time, float deviation) {
    m_meanTimeToLive = time;
    m_meanTimeToLiveDeviation = deviation;
}

void ParticleMode::setAppearance(m3g::AppearanceBase* appearance) {
    m_appearance = appearance;
}

m3g::AppearanceBase* ParticleMode::getAppearance() const {
    return m_appearance;
}

void ParticleMode::setScale(m3g::KeyframeSequence* sequence) {
    m_scale = sequence;
}

void ParticleMode::setRotation(m3g::KeyframeSequence* sequence) {
    m_rotation = sequence;
}

void ParticleMode::setCrop(m3g::KeyframeSequence* sequence) {
    m_crop = sequence;
}

void ParticleMode::setColor(m3g::KeyframeSequence* sequence) {
    m_color = sequence;
}

ParticleMode::ParticleMode(m3g::AppearanceBase* appearance)
    : m_meanTimeToLive(0.0f), m_meanTimeToLiveDeviation(0.0f) {
    setMeanTimeToLive(1000.0f, 100.0f);
    setScale(NULL);
    setRotation(NULL);
    setColor(NULL);
    setAppearance(appearance);
}

ParticleMode::~ParticleMode() {
}

float ParticleMode::getMeanTimeToLive() const {
    return m_meanTimeToLive;
}

float ParticleMode::getMeanTimeToLiveDeviation() const {
    return m_meanTimeToLiveDeviation;
}

m3g::KeyframeSequence* ParticleMode::getCrop() const {
    return m_crop;
}

m3g::KeyframeSequence* ParticleMode::getColor() const {
    return m_color;
}

m3g::KeyframeSequence* ParticleMode::getScale() const {
    return m_scale;
}

m3g::KeyframeSequence* ParticleMode::getRotation() const {
    return m_rotation;
}

}
