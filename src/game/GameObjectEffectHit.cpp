#include "game/GameObjectEffectHit.h"

#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/M3GAssets.h"
#include "game/MEdgeMap.h"
#include "game/particles/EmissionMode.h"
#include "game/particles/Emitter.h"
#include "game/particles/ParticleEffect.h"
#include "game/particles/ParticleMode.h"
#include "game/SceneGame.h"
#include "m3g/KeyframeSequence.h"
#include "m3g/Node.h"
#include "m3g/Transform.h"

GameObjectEffectHit::GameObjectEffectHit(MEdgeMap* map, m3g::Node* node)
    : GameObject(map, TYPE, 0.0f, 0.0f, 0.0f), m_node(node) {
    m_timeLeft = 1000;

    m3g::AppearanceBase* appearance = AppEngine::getM3GAssets()->loadTexturedAppearance(115, 4);
    particles::ParticleMode* mode = new particles::ParticleMode(appearance);
    mode->setMeanTimeToLive(175.0f, 75.0f);

    static const float scaleKeys[] = {0.0f, 1.0f, 2.5f};
    m3g::KeyframeSequence* scale = new m3g::KeyframeSequence(3, 1, m3g::KeyframeSequence::LINEAR);
    scale->setKeyframe(0, 0, &scaleKeys[0]);
    scale->setKeyframe(1, 50, &scaleKeys[1]);
    scale->setKeyframe(2, 250, &scaleKeys[2]);
    scale->setDuration(251);
    mode->setScale(scale);

    m3g::KeyframeSequence* rotation = new m3g::KeyframeSequence(2, 1, m3g::KeyframeSequence::LINEAR);
    float rotationKeys[] = {0.0f, 0.0f};
    rotation->setKeyframe(0, 0, &rotationKeys[0]);
    rotation->setKeyframe(1, 250, &rotationKeys[1]);
    rotation->setDuration(251);
    mode->setRotation(rotation);

    static const float cropKeys[] = {0.0f, 0.25f, 0.25f, 0.25f};
    m3g::KeyframeSequence* crop = new m3g::KeyframeSequence(1, 4, m3g::KeyframeSequence::STEP);
    crop->setKeyframe(0, 0, cropKeys);
    crop->setDuration(251);
    mode->setCrop(crop);

    static const float colorKeys[] = {
        1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
    };
    m3g::KeyframeSequence* color = new m3g::KeyframeSequence(3, 4, m3g::KeyframeSequence::LINEAR);
    color->setKeyframe(0, 0, &colorKeys[0]);
    color->setKeyframe(1, 50, &colorKeys[4]);
    color->setKeyframe(2, 250, &colorKeys[8]);
    color->setDuration(251);
    mode->setColor(color);

    m_emission = new particles::EmissionMode();
    m_emission->setRate(600.0f);
    m_emission->setSpeed(3.0f, 2.0f);
    m_emission->setSpreadAngle(90.0f, 90.0f);

    particles::Emitter* emitter =
        particles::Emitter::createEmitter(particles::Emitter::PARTICLES_QUAD, 1, mode, m_emission);
    m_effect = new particles::ParticleEffect(emitter);
    M3GAssets::addNode(AppEngine::getCanvas()->getSceneGame()->getM3GWorld(), m_effect);
    updatePosition();
}

void GameObjectEffectHit::updatePosition() {
    m3g::Group* world = AppEngine::getCanvas()->getSceneGame()->getM3GWorld();
    m3g::Transform transform;
    m_node->getTransformTo(world, &transform);
    float pos[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    transform.transform(pos, 4);
    m_effect->setTranslation(pos[0], pos[1], pos[2]);
}

void GameObjectEffectHit::update(int dt) {
    m_effect->updateStep(dt, NULL, NULL);
    m_emission->setRate(0.0f);
    m_timeLeft -= dt;
    if (m_timeLeft < 0) {
        m_map->removeObject(this);
        return;
    }
    updatePosition();
}

GameObjectEffectHit::~GameObjectEffectHit() {
    AppEngine::getCanvas()->getSceneGame()->getM3GWorld()->removeChild(m_effect);
}
