#include "game/GameObjectPlayer.h"

#include <math.h>

#include "game/AchievementData.h"
#include "game/AnimationManager3D.h"
#include "game/AnimPlayer3D.h"
#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectCheckpoint.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectEffectHit.h"
#include "game/GameObjectNPC.h"
#include "game/GameObjectPolice.h"
#include "game/GameObjectRunnerData.h"
#include "game/GameObjectZipLine.h"
#include "game/GameScreen.h"
#include "game/GhostAnimationRecorder.h"
#include "game/GhostKeyframe.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/M3GAssets.h"
#include "game/MathExt.h"
#include "game/MathLine.h"
#include "game/MathOrthoBox.h"
#include "game/MEdgeMap.h"
#include "game/particles/EmissionMode.h"
#include "game/particles/Emitter.h"
#include "game/particles/ParticleEffect.h"
#include "game/particles/ParticleMode.h"
#include "game/SceneGame.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "game/SoundSequencer.h"
#include "m3g/KeyframeSequence.h"
#ifdef MEDECOMP_PORT
namespace m3g {
class Node;
}
extern "C" int medecomp_player_model(int levelModel);
extern "C" void medecomp_dress_player(m3g::Node* root);
#endif
#include "m3g/Node.h"
#include "m3g/Transform.h"
#include "runtime/midp/Math.h"

static bool isSlideSoundState(GameObjectPlayer::PlayerState state) {
    return state == GameObjectPlayer::STATE_SLIDING || state == GameObjectPlayer::STATE_WALL_SLIDE ||
           state == GameObjectPlayer::STATE_RAMP_SLIDE;
}

static const int TYPE_ZIPLINE = 17;

static const int MAP_FLAG_BALANCE = 2;

static const float RUN_SPEED = 7.0f;

static const float SPRINT_SPEED = 10.0f;

static const int TYPE_SWING = 18;

static const int FLAG_PLAYER = 1;

static const int FLAG_POLICE = 4;

int GameObjectPlayer::getPlayerIndex() {
    return m_playerIndex;
}

int GameObjectPlayer::getNumCollectablesCollected() {
    return m_numCollectables;
}

GhostAnimationRecorder* GameObjectPlayer::getGhostAnimation() {
    return m_ghostRecorder;
}

void GameObjectPlayer::setZOffset(float offset) {
    m_zOffset = offset;
    m_zOffsetFilter.setSteadyState(offset);
}

void GameObjectPlayer::resetNumCollectablesCollected() {
    m_numCollectables = 0;
}

void GameObjectPlayer::incNumCollectablesCollected() {
    m_numCollectables++;
}

void GameObjectPlayer::resetCommon() {
    m_gestures = 0;
    m_lastCollidedObject = NULL;
    m_collisionCooldown = 0;
    m_health = 10.0f;
    m_healthRegenDelay = 0;
    setNoVelocity();
    setVisual(VISUAL_SOLO, 1);
}

int GameObjectPlayer::getTapGesture() {
    return m_state == STATE_HANG ? 0x40 : 0x10;
}

void GameObjectPlayer::updateHealth(int dt) {
    if (m_healthRegenDelay > 0 || m_health >= 10.0f) {
        m_healthRegenDelay -= dt;
        return;
    }
    m_health += 3.3f * 0.001f * dt;
    if (m_health > 10.0f) {
        m_health = 10.0f;
    }
}

void GameObjectPlayer::clearInvalidGestures() {
    switch (m_state) {
    case STATE_BALANCE_FELL:
    case STATE_CLAMBER:
    case STATE_HANG_CLAMBER:
        clearAllGesturesExcept(0x80);
        break;
    case STATE_ROLLING:
    case STATE_SCALE_JUMP:
    case STATE_SCALE_HAUL_UP:
        clearAllGesturesExcept(0x8f);
        break;
    case STATE_FALLING_VERTICALLY:
    case STATE_FALLING_FORWARDS:
        clearAllGesturesExcept(0x20);
        break;
    default:
        clearAllGestures();
        break;
    }
}

void GameObjectPlayer::deinitLineMovement() {
    m_lineMovement = false;
}

void GameObjectPlayer::initStateRunning(PlayerState prev) {
    float speed = getForwardVelocity() * getFacingDir();
    if (speed >= 7.0f) {
        m_maxSpeed = 10.0f;
        m_acceleration = 3.0f;
    } else {
        m_maxSpeed = 7.0f;
        m_acceleration = 3.0f;
        m_runningTimer = -1;
        setForwardVelocity(getFacingDir() * midp::Math::max(speed, 5.0f));
    }
}

void GameObjectPlayer::initStateRunningTurning(PlayerState prev) {
    setVisual(VISUAL_SOLO, 2);
    m_maxSpeed = getForwardVelocity();
}

void GameObjectPlayer::initStateBalanceFell(PlayerState prev) {
    setVisual(VISUAL_SOLO, 4);
    setNoVelocity();
    m_noClipMovement = true;
    clearGesture(0x80);
}

void GameObjectPlayer::deinitStateBalanceFell() {
    m_noClipMovement = false;
}

void GameObjectPlayer::initStateFalling(PlayerState prev) {
    m_landingRollTimer = 0;
    m_jumpVariant = false;
}

void GameObjectPlayer::initStateFallingVertically(PlayerState prev) {
    setForwardVelocity(0.0f);
    m_wallSlideBlock = 0;
}

void GameObjectPlayer::deinitStateFallingVertically() {
    m_ledgeGrabBlock = 0;
    m_wallSlideBlock = 0;
}

void GameObjectPlayer::deinitStateFallingForwards() {
    animStateTransition(ANIM_NONE);
}

void GameObjectPlayer::deinitStateCrash() {
    m_crashed = false;
}

void GameObjectPlayer::deinitStateWallClimb() {
    m_positionInterpolation.stop();
    clearGesture(0x80);
}

void GameObjectPlayer::deinitStateHang() {
    m_positionInterpolation.stop();
}

void GameObjectPlayer::deinitStateWallRun() {
    m_wallRunCooldown = 750;
    clearGesture(0xa0);
}

void GameObjectPlayer::initStateScaleJump(PlayerState prev) {
    clearGesture(0x8f);
}

void GameObjectPlayer::deinitStateScale() {
    m_position.z = m_positionInterpolation.getStartValue().z;
}

void GameObjectPlayer::initStateZipLineJumpTo(PlayerState prev) {
    m_noClipMovement = true;
}

void GameObjectPlayer::deinitStateZipLineJumpTo() {
    m_noClipMovement = false;
}

void GameObjectPlayer::initStateMeleedInTheFace(PlayerState prev) {
    m_lyingTimer = 3000;
    setNoVelocity();
}

int GameObjectPlayer::getCurrentMaterial() {
    if (m_state == STATE_RAMP_RUN_UP) {
        return m_rampShape->m_material;
    }
    return m_material;
}

void GameObjectPlayer::initStateIdle(PlayerState prev) {
    setForwardVelocity(0.0f);
    MathVector unused(m_position.x, m_position.y + 2.0f, m_position.z);
}

void GameObjectPlayer::snapPlayerToGround() {
    MathLine line(m_position, 0.0f, 1.0f, 0.0f);
    float moveOut = m_map->calculateCollisionMoveOut(line, 0);
    if (moveOut != 0.0f) {
        setUpVelocity(0.0f);
        m_position.y += moveOut;
    }
}

void GameObjectPlayer::deinitStateFlykick() {
    AppEngine::getAchievementData()->registerAttackEnd();
}

void GameObjectPlayer::deinitStateSliding() {
    m_achievementData->registerAttackEnd();
}

void GameObjectPlayer::initStateFlykick(PlayerState prev) {
    m_achievementData->registerAttackStart();
}

void GameObjectPlayer::initStateSliding(PlayerState prev) {
    m_maxSpeed = 5.0f;
    m_acceleration = 2.0f;
    m_slideUnderObstacle = false;
    m_achievementData->registerAttackStart();
}

void GameObjectPlayer::initStateDisarmingCop(PlayerState prev) {
    m_achievementData->registerDisarm();
}

void GameObjectPlayer::initStateZipLineFail(PlayerState prev) {
    m_velocity.set(0.0f, 0.0f, 0.0f);
    MathLine line(m_position, 0.0f, -1.0f, 0.0f);
    float distance = 0.0f;
    line.m_origin.x += getFacingDir() * m_zipLineEndOffset;
    if (m_map->calculateCollision(line, distance, 0)) {
        m_position.y -= distance;
    }
}

void GameObjectPlayer::deinitStateSwinging() {
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
    m_model->setTranslation(0.0f, 0.0f, 0.0f);
    m_objectNode->setOrientation(0.0f, 0.0f, 0.0f, 1.0f);
}

void GameObjectPlayer::initStateSwinging(PlayerState prev) {
    m_swingAmplitude = 1.0f;
    m_swingPhaseOffset = getFacingDir() > 0 ? 0.0f : midp::Math::PI;
    m_swingPhase = 0.0f;
    m_swingDir = getForwardVelocity() >= 0.0f ? 1 : -1;
    setNoVelocity();
    m_objectNode->setTranslation(m_position.x, m_position.y + 1.6f, m_position.z);
    m_model->setTranslation(0.0f, -1.6f, 0.0f);
    m_collisionCooldown = 2;
    clearGesture(0x80);
}

void GameObjectPlayer::initStateBalanceRunning(PlayerState prev) {
    m_acceleration = 10.0f;
    m_tiltNoiseFilter.setSteadyState(0.0f);
}

void GameObjectPlayer::unloadStateZipLineParticleEffect() {
    if (m_zipLineEffect != NULL) {
        AppEngine::getCanvas()->getSceneGame()->getM3GWorld()->removeChild(m_zipLineEffect);
        m_zipLineEffect = NULL;
    }
}

void GameObjectPlayer::deinitStateZipLine() {
    deinitLineMovement();
    m_zipLine = NULL;
    clearGesture(0x20);
    unloadStateZipLineParticleEffect();
}

void GameObjectPlayer::deinitStateRampAny() {
    m_rampShape = NULL;
    m_objectNode->setOrientation(0.0f, 0.0f, 0.0f, 1.0f);
    m_tiltNoiseFilter.setSteadyState(0.0f);
    deinitLineMovement();
    if (m_rampSlideEffect0 != NULL && m_rampSlideEffect0->getParent() != NULL) {
        m3g::Group* world = AppEngine::getCanvas()->getSceneGame()->getM3GWorld();
        world->removeChild(m_rampSlideEffect0);
        world->removeChild(m_rampSlideEffect1);
    }
}

void GameObjectPlayer::deinitStateWallSlide() {
    m_ledgeGrabBlock = 0;
    if (m_wallSlideEffect0 != NULL && m_wallSlideEffect0->getParent() != NULL) {
        m3g::Group* world = AppEngine::getCanvas()->getSceneGame()->getM3GWorld();
        world->removeChild(m_wallSlideEffect0);
        world->removeChild(m_wallSlideEffect1);
    }
}

void GameObjectPlayer::loadStateZipLineParticleEffect() {
    if (m_zipLineEffect != NULL) {
        return;
    }
    m3g::AppearanceBase* appearance = AppEngine::getM3GAssets()->loadTexturedAppearance(114, 4);
    particles::ParticleMode* mode = new particles::ParticleMode(appearance);
    mode->setMeanTimeToLive(200.0f, 100.0f);

    static const float scaleKeys[] = {0.0f, 0.0f, 0.25f, 0.8f, 0.0f, 0.0f};
    m3g::KeyframeSequence* scale = new m3g::KeyframeSequence(3, 2, m3g::KeyframeSequence::LINEAR);
    scale->setKeyframe(0, 0, &scaleKeys[0]);
    scale->setKeyframe(1, 200, &scaleKeys[2]);
    scale->setKeyframe(2, 1000, &scaleKeys[4]);
    scale->setDuration(1001);
    mode->setScale(scale);

    m3g::KeyframeSequence* rotation = new m3g::KeyframeSequence(2, 1, m3g::KeyframeSequence::LINEAR);
    float rotationKeys[] = {0.0f, 180.0f};
    rotation->setKeyframe(0, 0, &rotationKeys[0]);
    rotation->setKeyframe(1, 1000, &rotationKeys[1]);
    rotation->setDuration(1001);
    mode->setRotation(rotation);

    static const float cropKeys[] = {0.0f, 0.75f, 0.25f, 0.25f};
    m3g::KeyframeSequence* crop = new m3g::KeyframeSequence(1, 4, m3g::KeyframeSequence::STEP);
    crop->setKeyframe(0, 0, cropKeys);
    crop->setDuration(1001);
    mode->setCrop(crop);

    static const float colorKeys[] = {
        1.0f, 0.8f, 0.0f, 0.0f,
        1.0f, 0.8f, 0.0f, 1.0f,
        1.0f, 0.5f, 0.0f, 1.0f,
        1.0f, 0.5f, 0.0f, 0.0f,
    };
    m3g::KeyframeSequence* color = new m3g::KeyframeSequence(4, 4, m3g::KeyframeSequence::LINEAR);
    color->setKeyframe(0, 0, &colorKeys[0]);
    color->setKeyframe(1, 150, &colorKeys[4]);
    color->setKeyframe(2, 300, &colorKeys[8]);
    color->setKeyframe(3, 1000, &colorKeys[12]);
    color->setDuration(1001);
    mode->setColor(color);

    particles::EmissionMode* emission = new particles::EmissionMode();
    emission->setSpeed(7.0f, 1.0f);
    emission->setSpreadAngle(10.0f, 10.0f);

    particles::Emitter* emitter =
        particles::Emitter::createEmitter(particles::Emitter::PARTICLES_CYLINDER, 60, mode, emission);
    particles::ParticleEffect* effect = new particles::ParticleEffect(emitter);
    m_zipLineEffect = effect;
    AppEngine::getCanvas()->getSceneGame()->getM3GWorld()->addChild(effect);
}

void GameObjectPlayer::loadParticleEffectsForRampAndWallSlides() {
    if (m_rampSlideEffect0 != NULL) {
        return;
    }
    m3g::AppearanceBase* appearance = AppEngine::getM3GAssets()->loadTexturedAppearance(114, 4);
    particles::ParticleMode* mode = new particles::ParticleMode(appearance);
    mode->setMeanTimeToLive(300.0f, 150.0f);

    static const float scaleKeys[] = {0.5f, 1.0f, 0.5f};
    m3g::KeyframeSequence* scale = new m3g::KeyframeSequence(3, 1, m3g::KeyframeSequence::LINEAR);
    scale->setKeyframe(0, 0, &scaleKeys[0]);
    scale->setKeyframe(1, 100, &scaleKeys[1]);
    scale->setKeyframe(2, 500, &scaleKeys[2]);
    scale->setDuration(501);
    mode->setScale(scale);

    m3g::KeyframeSequence* rotation = new m3g::KeyframeSequence(2, 1, m3g::KeyframeSequence::LINEAR);
    float rotationKeys[] = {0.0f, 0.0f};
    rotation->setKeyframe(0, 0, &rotationKeys[0]);
    rotation->setKeyframe(1, 500, &rotationKeys[1]);
    rotation->setDuration(501);
    mode->setRotation(rotation);

    static const float cropKeys[] = {0.0f, 0.0f, 0.25f, 0.25f};
    m3g::KeyframeSequence* crop = new m3g::KeyframeSequence(1, 4, m3g::KeyframeSequence::STEP);
    crop->setKeyframe(0, 0, cropKeys);
    crop->setDuration(501);
    mode->setCrop(crop);

    static const float colorKeys[] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
    };
    m3g::KeyframeSequence* color = new m3g::KeyframeSequence(2, 4, m3g::KeyframeSequence::LINEAR);
    color->setKeyframe(0, 0, &colorKeys[0]);
    color->setKeyframe(1, 500, &colorKeys[4]);
    color->setDuration(501);
    mode->setColor(color);

    particles::EmissionMode* emission = new particles::EmissionMode();
    emission->setRate(10.0f);
    emission->setSpeed(1.0f, 1.0f);
    emission->setSpreadAngle(10.0f, 10.0f);

    particles::Emitter* emitter;
    emitter = particles::Emitter::createEmitter(particles::Emitter::PARTICLES_QUAD, 60, mode, emission);
    m_rampSlideEffect0 = new particles::ParticleEffect(emitter);
    emitter = particles::Emitter::createEmitter(particles::Emitter::PARTICLES_QUAD, 60, mode, emission);
    m_rampSlideEffect1 = new particles::ParticleEffect(emitter);
    emitter = particles::Emitter::createEmitter(particles::Emitter::PARTICLES_QUAD, 60, mode, emission);
    m_wallSlideEffect0 = new particles::ParticleEffect(emitter);
    emitter = particles::Emitter::createEmitter(particles::Emitter::PARTICLES_QUAD, 60, mode, emission);
    m_wallSlideEffect1 = new particles::ParticleEffect(emitter);
}

void GameObjectPlayer::completePositionInterpolation() {
    if (m_positionInterpolation.isFinished()) {
        return;
    }
    m_positionInterpolation.stop();
    m_position = m_positionInterpolation.getEndValue();
}

void GameObjectPlayer::initStateWallSlide(PlayerState prev) {
    loadParticleEffectsForRampAndWallSlides();
    if (m_wallSlideEffect0->getParent() == NULL) {
        m3g::Group* world = AppEngine::getCanvas()->getSceneGame()->getM3GWorld();
        world->addChild(m_wallSlideEffect0);
        world->addChild(m_wallSlideEffect1);
    }
    reverseFacingDir();
}

void GameObjectPlayer::initStateHang(PlayerState prev) {
    setNoVelocity();
    if (prev == STATE_WALL_SLIDE) {
        reverseFacingDir();
    }
}

void GameObjectPlayer::updateStateScaleHaulUp(int dt) {
    if (!isAnimating()) {
        return;
    }
    float t = calcAnimProgress();
    m_position.z = m_positionInterpolation.getStartValue().z * t +
                   m_positionInterpolation.getEndValue().z * (1.0f - t);
}

void GameObjectPlayer::updateAnimationStateRunning() {
    if (m_animState != ANIM_NONE || m_visualIndex == 18 || m_visualIndex == 30) {
        return;
    }
    float speed = getForwardVelocity() * getFacingDir();
    float weight = speed - 6.0f;
    if (speed >= 6.0f) {
        weight *= 0.25f;
    }
    set3WayAnimationBlendWeights(weight);
}

void GameObjectPlayer::updateStateSwinging(int dt) {
    float seconds = dt * 0.001f;
    m_swingPhase += seconds;
    m_swingAngle =
        m_swingAmplitude * midp::Math::sin(m_swingPhaseOffset + m_swingPhase * midp::Math::PI * 1.5151515f);
    m_swingDir =
        midp::Math::cos(m_swingPhaseOffset + m_swingPhase * midp::Math::PI * 1.5151515f) > 0.0f ? 1 : -1;

    float tilt = AppEngine::getCanvas()->getSceneGame()->getAccelerometerTiltVelocity();
    float push = midp::Math::max(0.0f, midp::Math::abs(m_swingDir * tilt * 0.7f));
    m_swingAmplitude =
        midp::Math::min(1.0f, midp::Math::max(0.05f, m_swingAmplitude + (push - 0.3f) * seconds));

    m_objectNode->setOrientation(m_swingAngle * 70.0f, 0.0f, 0.0f, 1.0f);
    set3WayAnimationBlendWeights(getFacingDir() * m_swingAngle);
}

void GameObjectPlayer::initStateHangClamber(PlayerState prev) {
    MathVector offset = getForwardDirection();
    offset *= (float)(getFacingDir() * 0.368);
    m_position += offset;
}

void GameObjectPlayer::updateMovementNoClip(float seconds) {
    MathVector delta(m_velocity);
    delta *= seconds;
    m_position += delta;
}

void GameObjectPlayer::deinitStateClamber() {
    m_shape->setPosition(m_position);
    if (m_map->intersects(m_shape, 0)) {
        const MathOrthoBox& bounds = m_shape->getLocalBounds();
        MathLine line(m_position.x, m_position.y + bounds.m_max.y * 0.5f, m_position.z, -1.0f, 0.0f, 0.0f);
        float tMin = 0.0f;
        float tMax = -bounds.m_min.x;
        if (m_map->intersects(line, 0, tMin, tMax)) {
            m_position.x += -bounds.m_min.x - tMin;
        } else {
            line.m_direction.x = 1.0f;
            tMax = bounds.m_max.x;
            if (m_map->intersects(line, 0, tMin, tMax)) {
                m_position.x += tMin - bounds.m_max.x;
            }
        }
    }

    MathVector move(m_position);
    move -= getPreOriginAnimPosNoOffset();
    move.y = 0.0f;
    if (m_originAnimPlayer->isAnimating()) {
        m_originAnimPlayer->setAnimating(false);
    } else {
        float speed = move.getLength() / (m_originAnimPlayer->getAnimDuration() * 0.001f);
        if (!isAnyGestureSet(0x80)) {
            speed = midp::Math::max(m_clamberExitSpeed, speed);
        }
        m_velocity = move;
        m_velocity.setLength(speed);
    }
    m_clamberJumpWindow = 300;
    m_clamberVisual = -1;
    m_clamberNextVisual = -1;
}

void GameObjectPlayer::checkpointActivated() {
    GameObjectCheckpoint* checkpoint =
        AppEngine::getCanvas()->getSceneGame()->getGameScreen(m_playerIndex)->getCheckpointObject();
    if (checkpoint == NULL) {
        return;
    }
    const MathOrthoBox& bounds = checkpoint->getGlobalBounds();
    MathLine line((bounds.m_min.x + bounds.m_max.x) * 0.5f, (bounds.m_min.y + bounds.m_max.y) * 0.5f,
                  (bounds.m_min.z + bounds.m_max.z) * 0.5f, 0.0f, -1.0f, 0.0f);
    float distance = 0.0f;
    if (m_map->calculateCollision(line, distance, 0)) {
        m_checkpointPosition = line.m_origin;
        m_checkpointPosition.y -= distance;
        m_checkpointFacingDir = (FacingDir)checkpoint->getPlayerFacingDir();
    }
}

void GameObjectPlayer::checkLedgeScale_CalculateJumpAndScale(const MathVector& ledge, MathVector& jump,
                                                             MathVector& scale) {
    MathVector offset;
    calculateOffsetAnimationOverallOffset(m_scaleVisual, offset);
    jump.set(ledge.x, ledge.y - offset.y, ledge.z + getRightDirection().z);
    scale.set(ledge.x + offset.x, ledge.y, ledge.z);
}

void GameObjectPlayer::initStateClamber(PlayerState prev) {
    clearGesture(0x80);
    MathVector offset;
    calculateOffsetAnimationOverallOffset(m_clamberVisual, offset);
    MathLine line(m_position.x + offset.x, m_position.y, m_position.z, 0.0f, 1.0f, 0.0f);

    float& offsetHeight = m_clamberOffsetHeight;
    float yOffset = getOriginAnimationYOffset(m_clamberVisual);
    offsetHeight = yOffset + yOffset;
    float tMin = 0.0f;
    if (!m_map->intersects(line, 0, tMin, offsetHeight)) {
        offsetHeight = yOffset;
    }
}

void GameObjectPlayer::initStateCrash(PlayerState prev) {
    SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
    m_crashed = true;
    setForwardVelocity(0.0f);
    scene->getGameScreen(m_playerIndex)->startCameraShake(0.5f, 300);
    scene->playerHurt(this, true);
}

void GameObjectPlayer::initStateRolling(PlayerState prev) {
    m_maxSpeed = midp::Math::max(8.0f, getFacingDir() * getForwardVelocity());
    m_acceleration = 60.0f;

    MathVector end(m_position);
    end.x += getFacingDir() * m_maxSpeed * getVisualDuration(VISUAL_SOLO, 18) * 0.001f;
    m_shapeNormal->setPosition(end);
    m_shapeCrouch->setPosition(end);
    m_rollIntoSlide = m_map->intersects(m_shapeNormal, 0) && !m_map->intersects(m_shapeCrouch, 0);
    m_shapeCrouch->setPosition(m_position);
}

void GameObjectPlayer::initStateStopping(PlayerState prev) {
    AnimationManager3D& animManager = AppEngine::getCanvas()->getAnimationManager3D();
    int duration = animManager.getAnimationDuration(9);
    m_speedInterpolation.start(getForwardSpeed(), 0.0f, duration, InterpolationTimed::LINEAR);
}

void GameObjectPlayer::deinitStateRunning() {
    m_disarmTimer = 0;
    AppEngine::getAchievementData()->registerSprintOff();
    if (m_animState == ANIM_HAUL_UP_TO_RUN) {
        m_animState = ANIM_NONE;
    }
}

bool GameObjectPlayer::checkWallrun() {
    CollShape* hit = NULL;
    if (m_map->intersects(m_position, 1, &hit)) {
        m_material = hit->m_material;
        return true;
    }
    const MathOrthoBox& local = m_shape->getLocalBounds();
    MathVector point(m_position.x, m_position.y + local.m_min.y + local.m_max.y * 0.5f, m_position.z);
    bool found = false;
    if (m_map->intersects(point, 1, &hit)) {
        m_material = hit->m_material;
        found = true;
    }
    return found;
}

void GameObjectPlayer::playerFail() {
    GameScreen* screen = AppEngine::getCanvas()->getSceneGame()->getGameScreen(m_playerIndex);
    int screenState = screen->getState();
    if (screenState == GameScreen::SCREEN_DEATH_FADE || screenState == GameScreen::SCREEN_MENU_MAIN) {
        return;
    }
    screen->stateTransition(GameScreen::SCREEN_DEATH_FADE);
}

bool GameObjectPlayer::checkDeathFall() {
    if (getUpVelocity() > -18.0f) {
        return false;
    }
    MathLine down(m_position.x, m_position.y, m_position.z, 0.0f, -1.0f, 0.0f);
    CollShape* shape = m_map->getAttributeShapeAt(down, 0);
    if (shape != NULL) {
        bool belowTop = m_position.y < shape->getBounds().m_max.y;
        if (shape->isRamp() && belowTop) {
            return false;
        }
    }
    playerFail();
    return true;
}

void GameObjectPlayer::updateStateFlykick(int dt) {
    if (!m_animPlayer->isAnimating()) {
        setVisual(VISUAL_SOLO, 36);
    }
    checkDeathFall();
}

void GameObjectPlayer::kickSomething() {
    m_map->addObject(new GameObjectEffectHit(m_map, m_effectNode106));
}

void GameObjectPlayer::updateParticleEffectForRampAndWallSlides(int dt, float rate, float speed,
                                                                particles::ParticleEffect* effect,
                                                                m3g::Node* node, float offsetX,
                                                                float offsetY) {
    if (effect == NULL || node == NULL) {
        return;
    }
    m3g::Group* world = AppEngine::getCanvas()->getSceneGame()->getM3GWorld();
    m3g::Transform transform;
    node->getTransformTo(world, &transform);
    float point[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    transform.transform(point, 4);
    effect->setTranslation(point[0] + offsetX, point[1] + offsetY, point[2] + 0.1f);

    int count = effect->getEmitterCount();
    for (int i = 0; i < count; i++) {
        particles::EmissionMode* mode = effect->getEmitter(i)->getEmissionMode();
        mode->setRate(rate);
        mode->setSpeed(speed, speed * 0.5f);
    }
    effect->updateStep(dt, NULL, NULL);
}

void GameObjectPlayer::updateLineMovement(float seconds) {
    MathVector step(m_lineDirection);
    step *= m_lineSpeed;
    m_velocity = step;
    step *= seconds;
    if (m_lineMovementMode == 1) {
        if (m_map->moveObject(m_position, m_velocity, seconds, m_shape)) {
            m_lineMovementMode = 2;
        }
    } else if (m_lineMovementMode == 0) {
        m_position += step;
    }
}

void GameObjectPlayer::initLineMovement(const MathVector& direction, float speed, bool collide) {
    m_lineMovement = true;
    m_lineSpeed = speed;
    m_lineDirection = direction;
    m_lineDirection.normalise();
    m_lineMovementMode = collide ? 1 : 0;
    m_velocity = m_lineDirection;
    m_velocity *= speed;
}

void GameObjectPlayer::initStateZipLine(PlayerState prev) {
    loadStateZipLineParticleEffect();
    initLineMovement(m_zipLine->getZipLineLine().m_direction, m_velocity.getLength(), true);
    m_tiltNoiseFilter.setSteadyState(0.0f);
}

void GameObjectPlayer::initStateRampSlide(PlayerState prev) {
    loadParticleEffectsForRampAndWallSlides();
    if (m_rampSlideEffect0->getParent() == NULL) {
        m3g::Group* world = AppEngine::getCanvas()->getSceneGame()->getM3GWorld();
        world->addChild(m_rampSlideEffect0);
        world->addChild(m_rampSlideEffect1);
    }

    if (m_velocity.y > 0.0f) {
        m_velocity.reverse();
    }
    if ((getForwardVelocity() < 0.0f) != (getFacingDir() == FACING_LEFT)) {
        reverseFacingDir();
    }

    const MathOrthoBox& local = m_shape->getLocalBounds();
    {
        MathLine probe(m_position, 0.0f, -1.0f, 0.0f);
        probe.m_origin.y += local.m_max.y * 0.5f;
        m_rampShape = m_map->getAttributeShapeAt(probe, 0);
        if (m_rampShape == NULL || !m_rampShape->isRamp()) {
            probe.m_origin.x += getFacingDir() == FACING_LEFT ? local.m_max.x : local.m_min.x;
            m_rampShape = m_map->getAttributeShapeAt(probe, 0);
        }
    }

    const MathOrthoBox& bounds = m_rampShape->getBounds();
    MathLine slope;
    float endX;
    if (getFacingDir() == FACING_RIGHT) {
        slope.m_origin.x = bounds.m_min.x;
        slope.m_origin.y = bounds.m_max.y;
        endX = bounds.m_max.x;
    } else {
        slope.m_origin.x = bounds.m_max.x;
        slope.m_origin.y = bounds.m_max.y;
        endX = bounds.m_min.x;
    }
    slope.m_direction.x = endX - slope.m_origin.x;
    slope.m_direction.y = bounds.m_min.y - slope.m_origin.y;
    initLineMovement(slope.m_direction, 10.5f, false);
    m_position.y = slope.calculateYatT(slope.calculateTatX(m_position.x)) + 0.01f;

    float forward = getForwardVelocity();
    float angle = MathExt::atan2(m_velocity.y, m_velocity.x);
    float degrees;
    float effectOffset;
    if (forward >= 0.0f) {
        degrees = midp::Math::toDegrees(angle);
        m_objectNode->setOrientation(degrees, 0.0f, 0.0f, 1.0f);
        effectOffset = 80.0f;
    } else {
        degrees = midp::Math::toDegrees(angle + midp::Math::PI);
        m_objectNode->setOrientation(degrees, 0.0f, 0.0f, 1.0f);
        effectOffset = -80.0f;
    }
    m_rampSlideEffect0->setOrientation(degrees + effectOffset, 0.0f, 0.0f, 1.0f);
    m_rampSlideEffect1->setOrientation(degrees + effectOffset, 0.0f, 0.0f, 1.0f);
    m_tiltNoiseFilter.setSteadyState(0.0f);
}

void GameObjectPlayer::initStateRampRunUp(PlayerState prev) {
    if (m_velocity.y < 0.0f) {
        m_velocity.reverse();
    }

    const MathOrthoBox& local = m_shape->getLocalBounds();
    {
        MathLine probe(m_position, 0.0f, -1.0f, 0.0f);
        probe.m_origin.y += local.m_max.y * 0.5f;
        m_rampShape = m_map->getAttributeShapeAt(probe, 0);
        if (m_rampShape == NULL || !m_rampShape->isRamp()) {
            probe.m_origin.x += getFacingDir() == FACING_LEFT ? local.m_min.x : local.m_max.x;
            m_rampShape = m_map->getAttributeShapeAt(probe, 0);
        }
    }

    const MathOrthoBox& bounds = m_rampShape->getBounds();
    MathVector direction(bounds.m_max);
    direction -= bounds.m_min;
    direction.z = 0.0f;
    if (getFacingDir() == FACING_LEFT) {
        direction.x = -direction.x;
    }
    initLineMovement(direction, midp::Math::max(7.0f, m_velocity.getLength()), true);
}

void GameObjectPlayer::startPositionInterpolation(const MathVector& from, const MathVector& to, int duration,
                                                  InterpolationTimed::InterpolationType type, bool collide) {
    m_positionInterpolation.start(from, to, duration, type);
    m_positionInterpCollide = collide;
}

void GameObjectPlayer::initStateWallClimb(PlayerState prev) {
    AnimationManager3D& animManager = AppEngine::getCanvas()->getAnimationManager3D();
    int duration = animManager.getAnimationDuration(AppEngine::getGameObjectRunnerData()->getSoloAnim(20).anim);
    MathVector from(m_position);
    MathVector to(m_position);
    to.y += 3.5f;
    startPositionInterpolation(from, to, duration, InterpolationTimed::EASE_OUT_QUAD, true);
}

void GameObjectPlayer::startPositionLinearInterpolation(const MathVector& from, const MathVector& to,
                                                        int duration, bool collide) {
    m_positionInterpolation.start(from, to, duration, InterpolationTimed::LINEAR);
    m_positionInterpCollide = collide;
}

float GameObjectPlayer::getAccelerometerTilt(bool withNoise) {
    if (AppEngine::getLevelData()->isMultiplayerMode()) {
        return withNoise ? m_tiltNoiseFilter.getFilteredValue() * 0.5f : 0.0f;
    }
    float tilt = AppEngine::getCanvas()->getSceneGame()->getAccelerometerTilt();
    float noise = withNoise ? m_tiltNoiseFilter.getFilteredValue() * 1.1f : 0.0f;
    return midp::Math::max(midp::Math::min(1.0f, tilt * 4.0f + noise), -1.0f);
}

void GameObjectPlayer::updateVelocity(int dt) {
    switch (m_state) {
    case STATE_IDLE:
        setForwardVelocity(0.0f);
        break;

    case STATE_BALANCE_RUNNING: {
        float tilt = getAccelerometerTilt(true);
        set3WayAnimationBlendWeights(getFacingDir() * -tilt);
        m_maxSpeed = 7.0f - midp::Math::abs(tilt);
    }
    case STATE_RUNNING:
    case STATE_ROLLING:
    case STATE_SLIDING: {
        float seconds = dt * 0.001f;
        float speed = getFacingDir() * getForwardVelocity();
        if (speed < m_maxSpeed) {
            speed += m_acceleration * seconds;
            if (speed >= m_maxSpeed) {
                speed = m_maxSpeed;
            }
        } else if (speed > m_maxSpeed) {
            speed -= m_acceleration * seconds;
            if (speed <= m_maxSpeed) {
                speed = m_maxSpeed;
            }
        }
        setForwardVelocity(getFacingDir() * speed);
        break;
    }

    case STATE_STOPPING:
        setForwardSpeed(m_speedInterpolation.getCurrentValue());
        break;

    case STATE_WALL_CLIMB:
        setUpVelocity(4.7f);
        break;
    case STATE_WALL_RUN:
        setUpVelocity(1.5f);
        break;
    case STATE_WALL_SLIDE:
        setUpVelocity(-5.0f);
        break;

    case STATE_RAMP_SLIDE: {
        float tilt = getAccelerometerTilt(true);
        set3WayAnimationBlendWeights(getFacingDir() * -tilt);
        m_lineSpeed = midp::Math::abs(tilt) * -2.0f + 12.0f;
        break;
    }
    case STATE_ZIPLINE: {
        float tilt = getAccelerometerTilt(true);
        set3WayAnimationBlendWeights(getFacingDir() * -tilt);
        m_lineSpeed = midp::Math::abs(tilt) * -2.0f + 13.0f;
        break;
    }

    default:
        break;
    }
}

void GameObjectPlayer::updateAccelerometerNoise(int dt) {
    m_accelerometerNoiseTimer -= dt;
    if (m_accelerometerNoiseTimer < 0) {
        m_tiltNoiseFilter.setTargetValue(AppEngine::getCanvas()->randFloat(-1.0f, 1.0f));
        m_accelerometerNoiseTimer = 2100;
    }
    m_tiltNoiseFilter.update(dt);
}

void GameObjectPlayer::setupAnimation() {
    if (!m_animDirty && m_animSetupState == m_state) {
        return;
    }
    m_animDirty = false;
    m_animSetupState = m_state;
    m_originAnimPlayer->setAnimating(false);
    if (m_animPlayer == NULL) {
        return;
    }

    switch (static_cast<int>(m_animState)) {
    case ANIM_NONE:
        break;
    case ANIM_LANDING:
        setVisual(VISUAL_SOLO, 7);
        return;
    case ANIM_VAULT_JUMP:
        setVisual(VISUAL_SOLO, 8);
        return;
    case ANIM_BOOST_JUMP:
        setVisual(VISUAL_SOLO, 9);
        return;
    case ANIM_BOOST_JUMP_END:
        setVisual(VISUAL_SOLO, 10);
        return;
    case ANIM_HAUL_UP_TO_RUN:
        setVisual(VISUAL_SOLO, 11);
        return;
    case ANIM_DROP_TO_WALL_SLIDE:
        setVisual(VISUAL_SOLO, 25);
        return;
    case ANIM_WALL_KICK:
        setVisual(VISUAL_SOLO, 24);
        return;
    case 8:
        setVisual(VISUAL_SOLO, 37);
        return;
    default:
        return;
    }

    switch (m_state) {
    case STATE_START: {
        const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();
        setVisual(VISUAL_SOLO, level->getIntroFaithAnim(m_playerIndex));
        break;
    }
    case STATE_IDLE:
        if (getVisualType() == VISUAL_SOLO && getVisualIndex() == 28) {
            setVisual(VISUAL_SOLO, 30);
        } else {
            setVisual(VISUAL_SOLO, 0);
        }
        break;
    case STATE_RUNNING:
        if (getVisualType() == VISUAL_SOLO && getVisualIndex() == 28) {
            setVisual(VISUAL_SOLO, 30);
        } else {
            setVisual(VISUAL_BLEND3, 0);
        }
        updateAnimationStateRunning();
        break;
    case STATE_RUNNING_TURNING:
    case STATE_BALANCE_FELL:
    case STATE_DEAD:
        break;
    case STATE_BALANCE_RUNNING:
        setVisual(VISUAL_BLEND3, 1);
        break;
    case STATE_STOPPING:
        setVisual(VISUAL_SOLO, 12);
        break;
    case STATE_FALLING_VERTICALLY:
    case STATE_FALLING_FORWARDS:
    case STATE_10:
    case STATE_SCALE_JUMP:
        if (getUpVelocity() > 0.0f) {
            if (m_jumpVariant) {
                setVisual(VISUAL_SOLO, 16);
            } else if (getVisualType() != VISUAL_SOLO || getVisualIndex() != 16) {
                setVisual(VISUAL_SOLO, 15);
            }
        } else {
            setVisual(VISUAL_SOLO, 17);
        }
        break;
    case STATE_ROLLING:
        setVisual(VISUAL_SOLO, 18);
        break;
    case STATE_CRASH:
        setVisual(VISUAL_SOLO, 19);
        break;
    case STATE_WALL_CLIMB:
        setVisual(VISUAL_SOLO, 20);
        break;
    case STATE_CLAMBER:
        if (m_clamberNextVisual != -1 && getVisualType() == VISUAL_ORIGIN && getVisualIndex() == m_clamberVisual) {
            setVisual(VISUAL_ORIGIN, m_clamberNextVisual);
        } else {
            setOffsetVisual(m_clamberVisual, m_clamberOffsetHeight);
        }
        break;
    case STATE_HANG:
        setVisual(VISUAL_SOLO, 26);
        break;
    case STATE_HANG_CLAMBER:
        setVisual(VISUAL_ORIGIN, 5);
        break;
    case STATE_SLIDING: {
        int index = getVisualIndex();
        if (m_slideUnderObstacle) {
            setVisual(VISUAL_SOLO, 29);
        } else if (getVisualType() == VISUAL_SOLO && (index == 18 || index == 27)) {
            setVisual(VISUAL_SOLO, 28);
        } else {
            setVisual(VISUAL_SOLO, 27);
        }
        break;
    }
    case STATE_WALL_RUN:
        setVisual(VISUAL_SOLO, getFacingDir() == FACING_LEFT ? 21 : 22);
        break;
    case STATE_WALL_SLIDE:
        setVisual(VISUAL_SOLO, 23);
        break;
    case STATE_SWINGING:
        setVisual(VISUAL_BLEND3, 2);
        break;
    case STATE_SCALE_HAUL_UP: {
        bool standing = GameCommon::isZero(getForwardVelocity());
        FacingDir dir = getFacingDir();
        if (standing) {
            setVisual(VISUAL_ORIGIN, dir == FACING_LEFT ? 6 : 7);
        } else {
            setVisual(VISUAL_ORIGIN, dir == FACING_LEFT ? 8 : 9);
        }
        break;
    }
    case STATE_RAMP_RUN_UP:
        setVisual(VISUAL_SOLO, 14);
        break;
    case STATE_RAMP_SLIDE:
        setVisual(VISUAL_BLEND3, 3);
        break;
    case STATE_ZIPLINE_JUMP_TO:
        setVisual(VISUAL_SOLO, 31);
        break;
    case STATE_ZIPLINE:
        setVisual(VISUAL_BLEND3, 4);
        break;
    case STATE_ZIPLINE_FAIL:
        setVisual(VISUAL_SOLO, 32);
        break;
    case STATE_MELEED_IN_THE_FACE:
        setVisual(VISUAL_ORIGIN, 10);
        break;
    case STATE_DISARMING_COP:
        setNoVelocity();
        setVisual(VISUAL_ORIGIN, 11);
        break;
    case STATE_LYING:
        setVisual(VISUAL_SOLO, 33);
        break;
    case STATE_GETTING_UP:
        setVisual(VISUAL_SOLO, 34);
        break;
    case STATE_GETUP_MELEE:
        setVisual(VISUAL_ORIGIN, 12);
        break;
    case STATE_FLYKICK:
        setVisual(VISUAL_SOLO, 36);
        break;
    case STATE_DYING:
        setVisual(VISUAL_SOLO, 39);
        break;
    default:
        setVisual(VISUAL_SOLO, 0);
        break;
    }
}

void GameObjectPlayer::setupSoundSequence() {
    SoundSequencer& sequencer = AppEngine::getCanvas()->getSceneGame()->getSoundSequencer();
    sequencer.stopSequence(m_runSoundSequence);
    m_runSoundSequence = -1;

    if (m_animState == ANIM_LANDING) {
        sequencer.playSequence(14, NULL);
        return;
    }

    int type;
    switch (m_state) {
    case STATE_RUNNING:
    case STATE_BALANCE_RUNNING:
    case STATE_WALL_RUN:
    case STATE_RAMP_RUN_UP:
        type = getType();
        if (type == 1) {
            m_runSoundSequence = sequencer.playSequence(1, NULL);
        } else if (type == 0) {
            m_runSoundSequence = sequencer.playSequence(0, NULL);
        }
        break;
    case STATE_ROLLING:
        type = getType();
        if (type == 1) {
            sequencer.playSequence(12, NULL);
        } else if (type == 0) {
            sequencer.playSequence(6, NULL);
        }
        break;
    case STATE_CRASH:
        type = getType();
        if (type == 1) {
            sequencer.playSequence(13, NULL);
        } else if (type == 0) {
            sequencer.playSequence(7, NULL);
        }
        break;
    case STATE_WALL_CLIMB:
    case STATE_CLAMBER:
        if (sequencer.isSequencePlaying(m_climbSoundSequence)) {
            return;
        }
        type = getType();
        if (type == 1) {
            m_climbSoundSequence = sequencer.playSequence(8, NULL);
        } else if (type == 0) {
            m_climbSoundSequence = sequencer.playSequence(2, NULL);
        }
        break;
    default:
        break;
    }
}

void GameObjectPlayer::updateSound(int dt) {
    AppEngine::getCanvas()->getSoundManager().setListenerPosition(m_position.x, m_position.y, m_position.z);
    if (m_state == m_soundState && m_animState == m_soundAnimState) {
        return;
    }
    m_soundState = m_state;
    m_soundAnimState = m_animState;
    setupSoundSequence();
}

void GameObjectPlayer::updateGhost(int dt) {
    if (AppEngine::getCanvas()->getSceneGame()->getState() == 3) {
        return;
    }
    int visual = getVisual();
    m_ghostTime += dt;
    if (m_ghostTime < 500 && m_ghostKeyframe->m_visual == visual) {
        return;
    }
    float blendWeight = get3WayAnimationBlendWeights();
    GhostKeyframe* keyframe = new GhostKeyframe(m_ghostTime, m_position, visual, blendWeight);
    m_ghostKeyframe = keyframe;
    m_ghostRecorder->addKeyframe(keyframe);
    m_ghostTime = 0;
}

void GameObjectPlayer::stateTransition(PlayerState state) {
    if (m_state == state) {
        return;
    }

    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    bool slideNext = isSlideSoundState(state);
    if (isSlideSoundState(m_state)) {
        if (!slideNext) {
            sound.stopEvent(73);
            sound.playEvent(74, 0.8f);
        }
    } else if (slideNext) {
        sound.playEvent(72, 0.8f);
        sound.setLoopCountEvent(sound.playEvent(73, 0.8f), -1);
    }

    PlayerState current = getState();
    if (state == STATE_ZIPLINE) {
        if (current != STATE_ZIPLINE) {
            sound.playEvent(75, 1.0f);
            int loop = sound.playEvent(76, 1.0f);
            sound.setLoopCountEvent(loop, -1);
            sound.setVolumeEvent(loop, 0.1f);
        }
    } else if (current == STATE_ZIPLINE) {
        sound.stopEvent(76);
        sound.playEvent(77, 1.0f);
    }

    PlayerState prev = m_state;
    m_state = state;
    m_stateTime = 0;

    if (isCrouchState(state)) {
        m_shape = m_shapeCrouch;
    } else if (state == STATE_RAMP_RUN_UP && m_shape != m_shapeBox) {
        m_shape = m_shapeBox;
    } else if (m_shape != m_shapeNormal) {
        m_shapeNormal->setPosition(m_position);
        if (prev != STATE_NONE && prev != STATE_RAMP_RUN_UP && m_map->intersects(m_shapeNormal, 0)) {
            m_state = prev;
        } else {
            m_shape = m_shapeNormal;
        }
    }

    switch (prev) {
    case STATE_RUNNING:
        deinitStateRunning();
        break;
    case STATE_BALANCE_FELL:
        deinitStateBalanceFell();
        break;
    case STATE_FALLING_VERTICALLY:
        deinitStateFallingVertically();
        break;
    case STATE_FALLING_FORWARDS:
        deinitStateFallingForwards();
        break;
    case STATE_CRASH:
        deinitStateCrash();
        break;
    case STATE_WALL_CLIMB:
        deinitStateWallClimb();
        break;
    case STATE_CLAMBER:
        deinitStateClamber();
        break;
    case STATE_HANG:
        deinitStateHang();
        break;
    case STATE_SLIDING:
        deinitStateSliding();
        break;
    case STATE_WALL_RUN:
        deinitStateWallRun();
        break;
    case STATE_WALL_SLIDE:
        deinitStateWallSlide();
        break;
    case STATE_SWINGING:
        deinitStateSwinging();
        break;
    case STATE_SCALE_JUMP:
    case STATE_SCALE_HAUL_UP:
        deinitStateScale();
        break;
    case STATE_RAMP_RUN_UP:
    case STATE_RAMP_SLIDE:
        deinitStateRampAny();
        break;
    case STATE_ZIPLINE_JUMP_TO:
        deinitStateZipLineJumpTo();
        break;
    case STATE_ZIPLINE:
        deinitStateZipLine();
        break;
    case STATE_FLYKICK:
        deinitStateFlykick();
        break;
    default:
        break;
    }

    float verticalAcceleration;
    switch (m_state) {
    case STATE_BALANCE_FELL:
    case STATE_WALL_CLIMB:
    case STATE_HANG:
    case STATE_HANG_CLAMBER:
    case STATE_WALL_RUN:
    case STATE_WALL_SLIDE:
    case STATE_SWINGING:
    case STATE_SCALE_JUMP:
    case STATE_RAMP_RUN_UP:
    case STATE_RAMP_SLIDE:
    case STATE_ZIPLINE_JUMP_TO:
    case STATE_ZIPLINE:
    case STATE_ZIPLINE_FAIL:
    case STATE_MELEED_IN_THE_FACE:
    case STATE_DYING:
        verticalAcceleration = 0.0f;
        break;
    case STATE_FLYKICK:
        verticalAcceleration = -6.0f;
        break;
    default:
        verticalAcceleration = GRAVITY;
        break;
    }
    setVerticalAcceleration(verticalAcceleration);
    m_zOffsetFilter.setTargetValue(m_state == STATE_ZIPLINE_JUMP_TO || m_state == STATE_ZIPLINE ? 0.0f : m_zOffset);

    switch (m_state) {
    case STATE_IDLE:
        initStateIdle(prev);
        break;
    case STATE_RUNNING:
        initStateRunning(prev);
        break;
    case STATE_RUNNING_TURNING:
        initStateRunningTurning(prev);
        break;
    case STATE_BALANCE_RUNNING:
        initStateBalanceRunning(prev);
        break;
    case STATE_BALANCE_FELL:
        initStateBalanceFell(prev);
        break;
    case STATE_STOPPING:
        initStateStopping(prev);
        break;
    case STATE_FALLING_VERTICALLY:
        initStateFalling(prev);
        initStateFallingVertically(prev);
        break;
    case STATE_FALLING_FORWARDS:
        initStateFalling(prev);
        break;
    case STATE_ROLLING:
        initStateRolling(prev);
        break;
    case STATE_CRASH:
        initStateCrash(prev);
        break;
    case STATE_WALL_CLIMB:
        initStateWallClimb(prev);
        break;
    case STATE_CLAMBER:
        initStateClamber(prev);
        break;
    case STATE_HANG:
        initStateHang(prev);
        break;
    case STATE_HANG_CLAMBER:
        initStateHangClamber(prev);
        break;
    case STATE_SLIDING:
        initStateSliding(prev);
        break;
    case STATE_WALL_SLIDE:
        initStateWallSlide(prev);
        break;
    case STATE_SWINGING:
        initStateSwinging(prev);
        break;
    case STATE_SCALE_JUMP:
        initStateScaleJump(prev);
        break;
    case STATE_RAMP_RUN_UP:
        initStateRampRunUp(prev);
        break;
    case STATE_RAMP_SLIDE:
        initStateRampSlide(prev);
        break;
    case STATE_ZIPLINE_JUMP_TO:
        initStateZipLineJumpTo(prev);
        break;
    case STATE_ZIPLINE:
        initStateZipLine(prev);
        break;
    case STATE_ZIPLINE_FAIL:
        initStateZipLineFail(prev);
        break;
    case STATE_MELEED_IN_THE_FACE:
        initStateMeleedInTheFace(prev);
        break;
    case STATE_DISARMING_COP:
        initStateDisarmingCop(prev);
        break;
    case STATE_FLYKICK:
        initStateFlykick(prev);
        break;
    default:
        break;
    }
}

void GameObjectPlayer::die() {
    m_achievementData->registerDeath();
    stateTransition(STATE_DYING);
}

void GameObjectPlayer::hurt(float damage, bool knockdown) {
    if (m_state == STATE_DYING || m_state == STATE_DEAD || m_state == STATE_NONE) {
        return;
    }
    AppEngine* engine = AppEngine::getCanvas();
    m_health -= damage;
    m_healthRegenDelay = 3000;
    if (m_health <= 0.0f) {
        die();
    }
    engine->getSceneGame()->playerHurt(this, knockdown);
}

void GameObjectPlayer::updateStateDisarmingCop(int dt) {
    if (!m_animPlayer->isAnimating()) {
        stateTransition(STATE_RUNNING);
    }
}

void GameObjectPlayer::updateStateMeleedInTheFace(int dt) {
    if (checkDeathFall()) {
        return;
    }
    if (!m_animPlayer->isAnimating()) {
        stateTransition(STATE_LYING);
    }
}

void GameObjectPlayer::updateStateZipLineFail(int dt) {
    if (!isAnimating()) {
        stateTransition(STATE_IDLE);
    }
}

void GameObjectPlayer::updateStateZipLine(int dt) {
    MathVector moved(m_lastPosition.x - m_position.x, m_lastPosition.y - m_position.y,
                     m_lastPosition.z - m_position.z);
    m_achievementData->registerZiplineDist(moved.getLength());

    MathLine line(m_zipLine->getZipLineLine());
    m_zipLineEndOffset = m_zipLine->getEndOffset();
    if (m_lineMovementMode == 2) {
        stateTransition(STATE_FALLING_FORWARDS);
    } else {
        float endX = line.m_direction.x + line.m_origin.x;
        if (line.m_origin.x < endX) {
            float dropX = endX - m_zipLineEndOffset;
            if (dropX < m_position.x) {
                m_position.x = dropX;
                stateTransition(STATE_ZIPLINE_FAIL);
            }
        } else {
            float dropX = m_zipLineEndOffset + endX;
            if (m_position.x < dropX) {
                m_position.x = dropX;
                stateTransition(STATE_ZIPLINE_FAIL);
            }
        }
    }

    updateAccelerometerNoise(dt);
    if (m_zipLineEffect != NULL) {
        float tilt = midp::Math::abs(getAccelerometerTilt(true));
        particles::ParticleEffect* effect = m_zipLineEffect;
        int count = effect->getEmitterCount();
        for (int i = 0; i < count; i++) {
            particles::Emitter* emitter = effect->getEmitter(i);
            emitter->setOrientation(getFacingDir() * 70, 0.0f, 0.0f, 1.0f);
            particles::EmissionMode* mode = emitter->getEmissionMode();
            mode->setRate(tilt * 100.0f + 2.0f);
            mode->setSpeed(tilt * 7.0f, tilt);
        }
        effect->setTranslation(m_position.x, m_position.y + 1.58f, m_position.z);
        effect->updateStep(dt, NULL, NULL);
    }
}

void GameObjectPlayer::updateStateZipLineJumpTo(int dt) {
    if ((m_lastPosition.y < m_zipLineTarget.y) != (m_position.y < m_zipLineTarget.y)) {
        m_position = m_zipLineTarget;
        stateTransition(STATE_ZIPLINE);
    }
}

void GameObjectPlayer::updateStateScaleJump(int dt) {
    if (m_positionInterpolation.isFinished()) {
        stateTransition(STATE_SCALE_HAUL_UP);
    }
}

void GameObjectPlayer::updateStateWallRun(int dt) {
    m_achievementData->registerWallRunDist(midp::Math::abs(m_lastPosition.x - m_position.x));
    if (isAnyGestureSet(GESTURE_DOWN) ||
        (isAnyGestureSet(GESTURE_ANY_LEFT) && getFacingDir() == FACING_RIGHT) ||
        (isAnyGestureSet(GESTURE_ANY_RIGHT) && getFacingDir() == FACING_LEFT) ||
        m_stateTime >= 1000 || !checkWallrun()) {
        stateTransition(STATE_FALLING_FORWARDS);
    }
}

void GameObjectPlayer::updateStateSliding(int dt) {
    m_achievementData->registerSlideDist(midp::Math::abs(m_lastPosition.x - m_position.x));
    if (getForwardVelocity() * getFacingDir() == m_maxSpeed) {
        stateTransition(STATE_RUNNING);
    } else if (getVisualType() == VISUAL_SOLO && getVisualIndex() == 27) {
        float tMin = 0.1f;
        float tMax = 0.9f;
        MathLine up(m_position.x, m_position.y, m_position.z, 0.0f, 1.8f, 0.0f);
        if (m_map->intersects(up, 0, tMin, tMax)) {
            m_slideUnderObstacle = true;
            m_animDirty = true;
        }
    }
}

void GameObjectPlayer::updateStateCrash(int dt) {
    if (!m_animPlayer->isAnimating()) {
        stateTransition(STATE_IDLE);
    }
}

void GameObjectPlayer::updateStateStopping(int dt) {
    m_speedInterpolation.update(dt);
    if (m_speedInterpolation.isFinished()) {
        stateTransition(STATE_IDLE);
    }
}

void GameObjectPlayer::updateStateBalanceFell(int dt) {
    switch (getVisualIndex()) {
    case 6:
        if (!isAnimating()) {
            stateTransition(STATE_BALANCE_RUNNING);
        }
        break;
    case 5:
        if (isAnyGestureSet(GESTURE_UP)) {
            setVisual(VISUAL_SOLO, 6);
        }
        break;
    case 4:
        if (!isAnimating()) {
            setVisual(VISUAL_SOLO, 5);
        }
        break;
    }
}

void GameObjectPlayer::updateStateBalanceRunning(int dt) {
    m_achievementData->registerBalanceDist(midp::Math::abs(m_lastPosition.x - m_position.x));
    if (!m_map->intersects(m_position, MAP_FLAG_BALANCE)) {
        stateTransition(STATE_RUNNING);
        return;
    }
    updateAccelerometerNoise(dt);
    if (midp::Math::abs(getAccelerometerTilt(true)) > 0.99f && m_stateTime >= 500) {
        stateTransition(STATE_BALANCE_FELL);
    }
}

void GameObjectPlayer::updateStateRunningTurning(int dt) {
    int visual = getVisualIndex();
    bool animating = isAnimating();
    if (visual == 2) {
        if (!animating) {
            setVisual(VISUAL_SOLO, 3);
            reverseFacingDir();
            return;
        }
        setForwardVelocity(m_maxSpeed * (1.0f - calcAnimProgress()));
    } else {
        if (!animating) {
            stateTransition(STATE_RUNNING);
            return;
        }
        setForwardVelocity(getFacingDir() * calcAnimProgress() * RUN_SPEED);
    }
}

void GameObjectPlayer::updateStateRunning(int dt) {
    m_achievementData->registerRunDist(midp::Math::abs(m_lastPosition.x - m_position.x));
    if (m_map->intersects(m_position, MAP_FLAG_BALANCE)) {
        stateTransition(STATE_BALANCE_RUNNING);
        return;
    }
    float speed = getForwardVelocity() * getFacingDir();
    if (speed == RUN_SPEED) {
        if (m_runningTimer == -1) {
            m_runningTimer = 500;
        } else {
            m_runningTimer -= dt;
            if (m_runningTimer < 0) {
                m_maxSpeed = SPRINT_SPEED;
            }
        }
    } else if (speed == SPRINT_SPEED) {
        AppEngine::getAchievementData()->registerSprintOn(AppEngine::getCanvas()->getSceneGame()->getRaceTime());
    }
    updateAnimationStateRunning();
}

bool GameObjectPlayer::checkLedgeScale() {
    MathVector ahead = getForwardDirection();
    ahead *= getForwardVelocity() * 0.1f;

    MathLine up(m_position.x, m_position.y + m_shape->getLocalBounds().m_max.y, m_position.z, 0.0f, 1.0f, 0.0f);
    up.m_origin += ahead;
    float tMin = 0.0f;
    float tMax = 1.5f;
    if (!m_map->intersects(up, 0, tMin, tMax)) {
        return false;
    }
    if (tMin <= 0.0f) {
        return false;
    }
    if (tMax < 0.2f || tMax > 1.5f) {
        return false;
    }

    bool standing = GameCommon::isZero(getForwardVelocity());
    FacingDir dir = getFacingDir();
    if (standing) {
        m_scaleVisual = dir == FACING_LEFT ? 6 : 7;
        m_scaleNextState = STATE_IDLE;
        m_scaleAnimState = ANIM_NONE;
    } else {
        if (dir == FACING_LEFT) {
            m_scaleVisual = 8;
            m_scaleAnimState = ANIM_HAUL_UP_TO_RUN;
        } else {
            m_scaleVisual = 9;
            m_scaleAnimState = ANIM_NONE;
        }
        m_scaleNextState = STATE_RUNNING;
    }

    MathVector ledge(m_position);
    ledge += ahead;
    ledge.y = up.m_origin.y + tMax;
    m_shape->setPosition(ledge);
    if (m_map->intersects(m_shape, 0)) {
        return false;
    }

    MathVector jump;
    MathVector scale;
    checkLedgeScale_CalculateJumpAndScale(ledge, jump, scale);
    m_shape->setPosition(scale);
    if (m_map->intersects(m_shape, 0)) {
        return false;
    }

    MathLine down(scale, 0.0f, -1.0f, 0.0f);
    CollShape* ground = m_map->getAttributeShapeAt(down, 0);
    if (ground == NULL) {
        return false;
    }
    if (!GameCommon::compareFloats(scale.y, ground->getBounds().m_max.y)) {
        return false;
    }

    MathLine pathHigh(m_position.x, scale.y + m_shape->getLocalBounds().m_max.y * 0.5f, 0.0f,
                      scale.x - m_position.x, 0.0f, 0.0f);
    MathLine pathLow(m_position.x, m_position.y + m_shape->getLocalBounds().m_max.y * 0.5f, 0.0f,
                     scale.x - m_position.x, 0.0f, 0.0f);
    float highMin = 0.0f;
    float highMax = 1.0f;
    float lowMin = 0.0f;
    float lowMax = 1.0f;
    if (m_map->intersects(pathHigh, 0, highMin, highMax)) {
        return false;
    }
    if (m_map->intersects(pathLow, 0, lowMin, lowMax)) {
        return false;
    }

    startPositionLinearInterpolation(m_position, jump, 100, false);
    stateTransition(STATE_SCALE_JUMP);
    return true;
}

void GameObjectPlayer::animStoppedStateScaleHaulUp() {
    if (isAnyGestureSet(GESTURE_UP)) {
        if (!checkLedgeScale()) {
            stateTransition(m_scaleNextState);
            if (m_scaleNextState == STATE_RUNNING) {
                setForwardVelocity(getFacingDir() * RUN_SPEED);
            }
        }
    } else if (isAnyGestureSet(GESTURE_ANY_LEFT)) {
        setFacingDir(FACING_LEFT);
        stateTransition(STATE_RUNNING);
        setForwardVelocity(getFacingDir() * RUN_SPEED);
    } else if (isAnyGestureSet(GESTURE_ANY_RIGHT)) {
        setFacingDir(FACING_RIGHT);
        stateTransition(STATE_RUNNING);
        setForwardVelocity(getFacingDir() * RUN_SPEED);
    } else {
        stateTransition(m_scaleNextState);
        if (m_scaleNextState == STATE_RUNNING) {
            setForwardVelocity(getFacingDir() * RUN_SPEED);
        }
    }
    if (m_scaleAnimState != ANIM_NONE) {
        animStateTransition(m_scaleAnimState);
    }
}

bool GameObjectPlayer::checkJumpToZipLine() {
    MathVector forward = getForwardDirection();
    forward *= getFacingDir() * 2.0f;
    MathVector side = getRightDirection();
    side *= 0.5f;
    MathOrthoBox box(-side.x, 0.0f, -side.z, forward.x + side.x, 2.5f, forward.z + side.z);
    box += m_position;

    int count = m_map->getNumPlayerCheckObjects();
    for (int i = 0; i != count; i++) {
        GameObject* obj = m_map->getPlayerCheckObject(i);
        if (obj->getType() != TYPE_ZIPLINE) {
            continue;
        }
        GameObjectZipLine* zipLine = (GameObjectZipLine*)obj;
        const MathLine& line = zipLine->getZipLineLine();
        if ((line.m_direction.x <= 0.0f) == (getFacingDir() == FACING_RIGHT)) {
            continue;
        }
        float tMin = 0.0f;
        float tMax = 0.0f;
        if (box.intersects(line, &tMin, &tMax) && GameCommon::boundsIntersect(0.0f, 0.9f, tMin, tMax)) {
            m_zipLine = zipLine;
            line.calculatePointAtT(midp::Math::max(0.0f, tMin), m_zipLineTarget);
            m_zipLineTarget.y -= 1.59f;
            m_velocity.set(m_zipLineTarget.x - m_position.x, m_zipLineTarget.y - m_position.y,
                           m_zipLineTarget.z - m_position.z);
            m_velocity.setLength(13.0f);
            stateTransition(STATE_ZIPLINE_JUMP_TO);
            return true;
        }
    }
    return false;
}

void GameObjectPlayer::playerClimbLedgeAfterCheck(FacingDir dir, int visual, int nextVisual, float height) {
    m_clamberVisual = visual;
    m_clamberNextVisual = nextVisual;
    m_clamberExitSpeed = height;
    if (getFacingDir() != dir) {
        reverseFacingDir();
    }
    stateTransition(STATE_CLAMBER);
}

void GameObjectPlayer::playerFullClamber(FacingDir dir, CollShape* shape) {
    if (getUpVelocity() < 0.0f) {
        playerClimbLedgeAfterCheck(dir, 4, -1, 5.0f);
    } else if (m_state == STATE_FALLING_FORWARDS || m_state == STATE_WALL_CLIMB) {
        playerClimbLedgeAfterCheck(dir, 2, -1, 7.0f);
    } else {
        playerClimbLedgeAfterCheck(dir, 3, -1, 5.0f);
    }
}

void GameObjectPlayer::playerHalfClamber(FacingDir dir, CollShape* shape) {
    if (m_state == STATE_FALLING_FORWARDS || m_state == STATE_WALL_CLIMB) {
        if (dir == FACING_LEFT) {
            if (shape->getShapeType() == 2) {
                return;
            }
        } else if (dir == FACING_RIGHT) {
            if (shape->getShapeType() == 1) {
                return;
            }
        }
        playerClimbLedgeAfterCheck(dir, 0, -1, 7.0f);
    } else {
        playerClimbLedgeAfterCheck(dir, 1, -1, 5.0f);
    }
}

bool GameObjectPlayer::checkWallAction(FacingDir dir, bool wallClimb, int flags) {
    if (getUpVelocity() <= -18.0f) {
        return false;
    }
    MathVector front = getForwardDirection();
    front *= dir * 0.51f;
    front += m_position;
    MathLine up(front, 0.0f, 1.0f, 0.0f);
    const MathOrthoBox& bounds = m_shape->getLocalBounds();
    float tMin = bounds.m_min.y;
    float tMax = bounds.m_max.y;
    if (!m_map->intersects(up, 0, tMin, tMax)) {
        if (wallClimb) {
            MathVector step = getForwardDirection();
            step *= 0.1f;
            m_position += step;
            setUpVelocity(0.0f);
            stateTransition(STATE_RUNNING);
        }
        return false;
    }

    if (flags & WALL_CLAMBER_EXACT) {
        if (GameCommon::compareFloats(tMax, 0.5f) && getUpVelocity() >= 0.0f) {
            MathLine down(up);
            down.m_origin.y = down.m_origin.y + tMax + 0.01f;
            down.m_direction.y = -1.0f;
            playerHalfClamber(dir, m_map->getAttributeShapeAt(down, 0));
            return true;
        }
        if (GameCommon::compareFloats(tMax, 1.0f)) {
            MathLine down(up);
            down.m_origin.y = down.m_origin.y + tMax + 0.01f;
            down.m_direction.y = -1.0f;
            playerFullClamber(dir, m_map->getAttributeShapeAt(down, 0));
            return true;
        }
    }

    if ((flags & WALL_CLAMBER_LOW) && tMax > 0.0f && tMax <= 0.5f) {
        MathLine down(up);
        down.m_origin.y = down.m_origin.y + tMax + 0.01f;
        down.m_direction.y = -1.0f;
        playerHalfClamber(dir, m_map->getAttributeShapeAt(down, 0));
        return true;
    }

    if (flags & WALL_CLAMBER_PREDICT) {
        float nextHeight = m_lastPosition.y - m_position.y + tMax;
        if (nextHeight <= 0.5f && tMax >= 0.5f) {
            MathLine down(up);
            down.m_origin.y = down.m_origin.y + tMax + 0.01f;
            down.m_direction.y = -1.0f;
            playerHalfClamber(dir, m_map->getAttributeShapeAt(down, 0));
            return true;
        }
        if ((tMax <= 1.0f) != (nextHeight <= 1.0f)) {
            MathLine down(up);
            down.m_origin.y = down.m_origin.y + tMax + 0.01f;
            down.m_direction.y = -1.0f;
            playerFullClamber(dir, m_map->getAttributeShapeAt(down, 0));
            return true;
        }
    }

    if ((flags & WALL_HANG) && getUpVelocity() < 0.0f) {
        float nextHeight = m_lastPosition.y - m_position.y + tMax;
        if (tMax >= 1.0f && (tMax <= 1.9f || nextHeight <= 1.0f)) {
            MathLine below(m_position, 0.0f, -1.0f, 0.0f);
            float belowMin = 0.0f;
            float belowMax = 2.3f;
            if (!m_map->intersects(below, 0, belowMin, belowMax)) {
                MathVector hangPos(m_position.x, m_position.y + (tMax - 2.0f), m_position.z);
                startPositionInterpolation(m_position, hangPos, 100, InterpolationTimed::LINEAR, false);
                stateTransition(STATE_HANG);
                return true;
            }
        }
    }

    if (!(flags & WALL_CLIMB_SLIDE)) {
        return false;
    }
    if (tMax > 0.5f && tMax - tMin >= 1.0f && m_prevVelocity.x * getFacingDir() > 5.0f &&
        getUpVelocity() > 0.0f) {
        stateTransition(STATE_WALL_CLIMB);
        return true;
    }
    if (bounds.m_min.y > tMin && bounds.m_max.y <= tMax && getUpVelocity() <= 0.0f) {
        stateTransition(STATE_WALL_SLIDE);
        return true;
    }
    if (m_state != STATE_WALL_SLIDE || bounds.m_min.y <= tMin) {
        stateTransition(STATE_FALLING_VERTICALLY);
    }
    return false;
}

void GameObjectPlayer::updateStateWallSlide(int dt) {
    int flags = m_ledgeGrabBlock == 0 ? WALL_CLAMBER_LOW | WALL_CLAMBER_PREDICT | WALL_HANG | WALL_CLIMB_SLIDE
                              : WALL_CLIMB_SLIDE;
    checkWallAction((FacingDir)-getFacingDir(), false, flags);

    updateParticleEffectForRampAndWallSlides(dt, 25.0f, 1.0f, m_wallSlideEffect0, m_effectNode110, 0.0f, 0.2f);
    updateParticleEffectForRampAndWallSlides(dt, 25.0f, 1.0f, m_wallSlideEffect1, m_effectNode106, 0.0f, 0.2f);
    m_wallSlideEffect0->setOrientation(getFacingDir() * -10.0f, 0.0f, 0.0f, 1.0f);
    m_wallSlideEffect1->setOrientation(getFacingDir() * -10.0f, 0.0f, 0.0f, 1.0f);
}

void GameObjectPlayer::updateStateWallClimb(int dt) {
    m_achievementData->registerClimbDist(midp::Math::abs(m_lastPosition.y - m_position.y));
    if (m_positionInterpolation.isFinished()) {
        stateTransition(STATE_FALLING_VERTICALLY);
        setUpVelocity(0.0f);
        return;
    }
    checkWallAction(getFacingDir(), true, WALL_CLAMBER_PREDICT);
}

void GameObjectPlayer::updateStateFallingForwards(int dt) {
    float fall = m_lastPosition.y - m_position.y;
    if (fall > 0.0f) {
        m_achievementData->registerFallDist(fall);
    }
    if (checkDeathFall()) {
        return;
    }
    checkWallAction(getFacingDir(), false, WALL_CLAMBER_PREDICT);
    if (m_landingRollTimer > 0) {
        m_landingRollTimer -= dt;
    }
    if (m_collisionCooldown != 0) {
        if (--m_collisionCooldown == 0) {
            m_lastCollidedObject = NULL;
        }
    }
}

void GameObjectPlayer::updateStateFallingVertically(int dt) {
    float fall = m_lastPosition.y - m_position.y;
    if (fall > 0.0f) {
        m_achievementData->registerFallDist(fall);
    }
    if (checkDeathFall()) {
        return;
    }
    int flags = 0;
    if (m_ledgeGrabBlock == 0) {
        flags = WALL_CLAMBER_LOW | WALL_CLAMBER_PREDICT;
        if (getUpVelocity() < 0.0f) {
            flags |= WALL_HANG;
        }
    }
    if (m_wallSlideBlock == 0) {
        flags |= WALL_CLIMB_SLIDE;
    }
    if (flags != 0) {
        checkWallAction(getFacingDir(), false, flags);
    }
    if (m_landingRollTimer > 0) {
        m_landingRollTimer -= dt;
    }
    if (m_collisionCooldown != 0) {
        m_collisionCooldown--;
    }
}

void GameObjectPlayer::playerSwingLaunch() {
    if (m_swingDir != getFacingDir()) {
        return;
    }
    m_position.x += midp::Math::sin(m_swingAngle * midp::Math::PI * 0.0055555557f) * 0.64000005f;
    setForwardVelocity(getFacingDir() * 7.5f);
    setUpVelocity(5.0f);
    stateTransition(STATE_FALLING_FORWARDS);

    SoundSequencer& sequencer = AppEngine::getCanvas()->getSceneGame()->getSoundSequencer();
    switch (getType()) {
    case 0:
        sequencer.playSequence(5, NULL);
        break;
    case 1:
        sequencer.playSequence(11, NULL);
        break;
    }
}

void GameObjectPlayer::playerZipLine(GameObjectZipLine* zipLine) {
    m_zipLine = zipLine;
    stateTransition(STATE_ZIPLINE);
}

void GameObjectPlayer::playerJump(bool vault, bool longJump) {
    PlayerState prev = m_state;
    stateTransition(prev == STATE_IDLE ? STATE_FALLING_VERTICALLY : STATE_FALLING_FORWARDS);
    if (vault) {
        setForwardSpeed(midp::Math::max(getForwardSpeed(), 5.5f));
        setUpVelocity(8.15f);
        animStateTransition(ANIM_VAULT_JUMP);
        clearGesture(GESTURE_UP);
    } else if (m_map->intersects(m_position, 3)) {
        stateTransition(STATE_FALLING_FORWARDS);
        setForwardSpeed(12.0f);
        setUpVelocity(7.0f);
        animStateTransition(ANIM_BOOST_JUMP);
    } else if (prev == STATE_RAMP_SLIDE) {
        setForwardVelocity(getFacingDir() * 10.0f);
        setUpVelocity(5.0f);
    } else if (prev == STATE_RAMP_RUN_UP) {
        setForwardVelocity(getFacingDir() * 6.5f);
        setUpVelocity(6.5f);
    } else {
        setUpVelocity(5.0f);
    }
    clearGesture(GESTURE_JUMP);
    m_jumpVariant = longJump;

    SoundSequencer& sequencer = AppEngine::getCanvas()->getSceneGame()->getSoundSequencer();
    int type = getType();
    int id;
    if (vault) {
        if (type == 1) {
            id = 10;
        } else if (type == 0) {
            id = 4;
        } else {
            return;
        }
    } else {
        if (type == 1) {
            id = 9;
        } else if (type == 0) {
            id = 3;
        } else {
            return;
        }
    }
    sequencer.playSequence(id, NULL);
}

void GameObjectPlayer::animStoppedStateHangClamber() {
    if (isAnyGestureSet(GESTURE_UP)) {
        snapPlayerToGround();
        playerJump(true, true);
        m_ledgeGrabBlock = 1;
        return;
    }
    setForwardSpeed(5.0f);
    stateTransition(STATE_RUNNING);
}

void GameObjectPlayer::animStoppedStateClamber() {
    bool inNextVisual = getVisualType() == VISUAL_ORIGIN && getVisualIndex() == m_clamberNextVisual;
    if (!inNextVisual && isAnyGestureSet(GESTURE_UP)) {
        snapPlayerToGround();
        playerJump(true, true);
        m_ledgeGrabBlock = 1;
        return;
    }
    if (m_clamberNextVisual != -1 && getVisualType() == VISUAL_ORIGIN && getVisualIndex() == m_clamberVisual) {
        return;
    }

    const MathOrthoBox& bounds = m_shape->getLocalBounds();
    float top = m_position.y + bounds.m_max.y;
    MathLine down(m_position.x, top, m_position.z, 0.0f, -1.0f, 0.0f);
    float tMin = 0.0f;
    float tMax = bounds.m_max.y - bounds.m_min.y;
    if (m_map->intersects(down, 0, tMin, tMax)) {
        m_position.y = top - tMin;
    }
    stateTransition(STATE_RUNNING);
}

void GameObjectPlayer::updateStateRolling(int dt) {
    if (m_rollIntoSlide) {
        if (m_animPlayer->getAnimTime() > 300) {
            stateTransition(STATE_SLIDING);
        }
        return;
    }
    if (!m_animPlayer->isAnimating()) {
        stateTransition(STATE_RUNNING);
        return;
    }
    if (m_animPlayer->getAnimTime() > 600 && isAnyGestureSet(GESTURE_UP)) {
        snapPlayerToGround();
        playerJump(false, false);
        clearGesture(GESTURE_UP);
        return;
    }
    if (m_animPlayer->getAnimTime() > 600 && isAnyGestureSet(GESTURE_ANY_SIDE) && m_disarmCooldown <= 0) {
        m_disarmTimer = 1000;
        m_disarmCooldown = 2000;
    }
}

void GameObjectPlayer::meleeByPlayer() {
    m_immunityTimer = 2500;
    if (m_state == STATE_SCALE_HAUL_UP || m_state == STATE_MELEED_IN_THE_FACE) {
        return;
    }
    AppEngine* canvas = AppEngine::getCanvas();
    SoundManager& sound = canvas->getSoundManager();
    SceneGame* scene = canvas->getSceneGame();
    SoundEventPoolManager& pools = scene->getSoundPoolManager();
    stateTransition(STATE_MELEED_IN_THE_FACE);
    scene->playerHurt(this, true);
    sound.playEvent(pools.getRandomSoundEventID(5, 1, -1), 1.0f);
}

void GameObjectPlayer::disarmCop(GameObjectNPC* cop) {
    m_disarmTimer = 0;
    m_disarmCooldown = 0;
    stateTransition(STATE_DISARMING_COP);
}

void GameObjectPlayer::meleeByCop(GameObjectNPC* cop) {
    if (m_state == STATE_MELEED_IN_THE_FACE || m_state == STATE_SCALE_HAUL_UP) {
        return;
    }
    if ((m_state == STATE_FALLING_VERTICALLY || m_state == STATE_FALLING_FORWARDS) &&
        m_position.y >= cop->m_position.y + 1.2f) {
        return;
    }
    stateTransition(STATE_MELEED_IN_THE_FACE);
}

void GameObjectPlayer::hitHead() {
    if (m_state == STATE_WALL_CLIMB) {
        stateTransition(STATE_FALLING_VERTICALLY);
    }
}

void GameObjectPlayer::updateMovementLinearInterpolation(int dt) {
    m_positionInterpolation.update(dt);
    m_position = m_positionInterpolation.getCurrentValue();
    if (!m_positionInterpCollide) {
        return;
    }
    m_shape->setPosition(m_position);
    if (m_map->intersects(m_shape, 0)) {
        if (m_lastPosition.y < m_position.y) {
            hitHead();
        }
        m_position = m_lastPosition;
    }
}

void GameObjectPlayer::fallPlayer() {
    switch (m_state) {
    case STATE_IDLE:
        stateTransition(STATE_FALLING_VERTICALLY);
        break;

    case STATE_RUNNING:
    case STATE_SLIDING: {
        MathLine down(m_position.x, m_position.y, m_position.z, 0.0f, -1.0f, 0.0f);
        CollShape* shape = m_map->getAttributeShapeAt(down, 0);
        PlayerState next = STATE_FALLING_FORWARDS;
        if (shape != NULL) {
            float y = m_position.y;
            float top = shape->getBounds().m_max.y;
            FacingDir dir = getFacingDir();
            int shapeType = shape->getShapeType();
            bool downhill = (dir == FACING_LEFT && shapeType == 2) || (dir == FACING_RIGHT && shapeType == 1);
            if (shape->isRamp() && y + 0.5f >= top && y - 0.5f <= top && downhill) {
                next = STATE_RAMP_SLIDE;
            }
        }
        stateTransition(next);
        break;
    }

    case STATE_FALLING_VERTICALLY:
    case STATE_FALLING_FORWARDS:
        if (m_animPlayer->getAnimID() == 14 && getUpVelocity() <= 0.0f) {
            refreshAnimation();
        }
        break;

    case STATE_10:
    case STATE_CRASH:
    case STATE_WALL_CLIMB:
    case STATE_CLAMBER:
    case STATE_HANG:
    case STATE_WALL_RUN:
    case STATE_WALL_SLIDE:
    case STATE_SWINGING:
    case STATE_ZIPLINE:
    case STATE_MELEED_IN_THE_FACE:
    case STATE_LYING:
    case STATE_FLYKICK:
        break;

    default:
        stateTransition(STATE_FALLING_FORWARDS);
        break;
    }
}

void GameObjectPlayer::groundPlayer() {
    AppEngine::getAchievementData()->registerMapCollision();
    switch (m_state) {
    case STATE_FALLING_VERTICALLY:
    case STATE_FALLING_FORWARDS:
    case STATE_FLYKICK:
        if (m_landingRollTimer > 0) {
            stateTransition(STATE_ROLLING);
        } else if (getPrevUpVelocity() >= -8.0f) {
            animStateTransition(ANIM_LANDING);
            stateTransition(m_state == STATE_FALLING_VERTICALLY ? STATE_IDLE : STATE_RUNNING);
        } else {
            m_achievementData->registerBadLanding();
            stateTransition(STATE_CRASH);
        }
        break;
    case STATE_10:
        stateTransition(STATE_CRASH);
        break;
    case STATE_WALL_SLIDE:
        stateTransition(STATE_IDLE);
        break;
    case STATE_RAMP_RUN_UP:
    case STATE_RAMP_SLIDE:
        stateTransition(STATE_RUNNING);
        break;
    default:
        break;
    }
}

void GameObjectPlayer::updateStateRampSlide(int dt) {
    if (m_position.y < m_rampShape->getBounds().m_min.y) {
        MathLine up(m_position, 0.0f, 1.0f, 0.0f);
        m_position.y += m_map->calculateCollisionMoveOut(up, 0);
        groundPlayer();
    }

    float tilt = midp::Math::abs(getAccelerometerTilt(true));
    float rate = tilt * tilt * 28.0f + 2.0f;
    if (getFacingDir() == FACING_LEFT) {
        updateParticleEffectForRampAndWallSlides(dt, rate, tilt, m_rampSlideEffect0, m_effectNode106, 0.25f, 0.0f);
        updateParticleEffectForRampAndWallSlides(dt, rate, tilt, m_rampSlideEffect1, m_effectNode105, 0.25f, 0.0f);
    } else {
        updateParticleEffectForRampAndWallSlides(dt, rate, tilt, m_rampSlideEffect0, m_effectNode106, 0.1f, 0.3f);
        updateParticleEffectForRampAndWallSlides(dt, rate, tilt, m_rampSlideEffect1, m_effectNode105, 0.1f, 0.3f);
    }
    updateAccelerometerNoise(dt);
}

void GameObjectPlayer::updateStateRampRunUp(int dt) {
    if (m_lineMovementMode == 2) {
        stateTransition(STATE_WALL_CLIMB);
        return;
    }
    float top = m_rampShape->getBounds().m_max.y;
    if (m_position.y > top) {
        m_position.y = top;
        setUpVelocity(0.0f);
        groundPlayer();
    }
}

void GameObjectPlayer::hitWall() {
    AppEngine::getAchievementData()->registerMapCollision();
    if (m_state == STATE_FALLING_VERTICALLY) {
        return;
    }
    if (m_state == STATE_FALLING_FORWARDS) {
        checkWallAction(getFacingDir(), false,
                        WALL_CLAMBER_LOW | WALL_CLAMBER_PREDICT | WALL_HANG | WALL_CLIMB_SLIDE);
        return;
    }
    stateTransition(STATE_IDLE);
}

void GameObjectPlayer::updateState(int dt) {
    switch (m_state) {
    case STATE_RUNNING:
        updateStateRunning(dt);
        break;
    case STATE_RUNNING_TURNING:
        updateStateRunningTurning(dt);
        break;
    case STATE_BALANCE_RUNNING:
        updateStateBalanceRunning(dt);
        break;
    case STATE_BALANCE_FELL:
        updateStateBalanceFell(dt);
        break;
    case STATE_STOPPING:
        updateStateStopping(dt);
        break;
    case STATE_FALLING_VERTICALLY:
        updateStateFallingVertically(dt);
        break;
    case STATE_FALLING_FORWARDS:
        updateStateFallingForwards(dt);
        break;
    case STATE_ROLLING:
        updateStateRolling(dt);
        break;
    case STATE_CRASH:
        updateStateCrash(dt);
        break;
    case STATE_WALL_CLIMB:
        updateStateWallClimb(dt);
        break;
    case STATE_SLIDING:
        updateStateSliding(dt);
        break;
    case STATE_WALL_RUN:
        updateStateWallRun(dt);
        break;
    case STATE_WALL_SLIDE:
        updateStateWallSlide(dt);
        break;
    case STATE_SWINGING:
        updateStateSwinging(dt);
        break;
    case STATE_SCALE_JUMP:
        updateStateScaleJump(dt);
        break;
    case STATE_SCALE_HAUL_UP:
        updateStateScaleHaulUp(dt);
        break;
    case STATE_RAMP_RUN_UP:
        updateStateRampRunUp(dt);
        break;
    case STATE_RAMP_SLIDE:
        updateStateRampSlide(dt);
        break;
    case STATE_ZIPLINE_JUMP_TO:
        updateStateZipLineJumpTo(dt);
        break;
    case STATE_ZIPLINE:
        updateStateZipLine(dt);
        break;
    case STATE_ZIPLINE_FAIL:
        updateStateZipLineFail(dt);
        break;
    case STATE_MELEED_IN_THE_FACE:
        updateStateMeleedInTheFace(dt);
        break;
    case STATE_DISARMING_COP:
        updateStateDisarmingCop(dt);
        break;
    case STATE_LYING:
        if (checkDeathFall() || AppEngine::getLevelData()->isMultiplayerMode()) {
            break;
        }
        m_lyingTimer -= dt;
        if (m_lyingTimer <= 0) {
            playerFail();
        }
        break;
    case STATE_GETTING_UP:
        if (!m_animPlayer->isAnimating()) {
            stateTransition(STATE_IDLE);
        }
        break;
    case STATE_GETUP_MELEE:
        if (!m_animPlayer->isAnimating()) {
            stateTransition(STATE_RUNNING);
        }
        break;
    case STATE_FLYKICK:
        updateStateFlykick(dt);
        break;
    case STATE_DYING:
        if (!m_animPlayer->isAnimating()) {
            stateTransition(STATE_DEAD);
        }
        break;
    default:
        break;
    }
}

void GameObjectPlayer::updateMovementPhysics(float seconds) {
    float prevForwardVelocity = getForwardVelocity();
    float feetY = m_position.y + m_shape->getLocalBounds().m_min.y;
    MathVector normal;
    CollShape* floor = NULL;
    int hit = m_map->moveObjectPhysics(m_position, m_velocity, seconds, m_shape, &floor, normal);

    if (m_map->isInFallBox(this)) {
        playerFail();
    }
    if (m_map->isInPainBox(this)) {
        hurt(seconds * 6.9f, true);
    }

    if (hit & 0xc0) {
        if (getForwardVelocity() * getFacingDir() < 0.0f) {
            m_velocity.reverse();
        }
        stateTransition(getUpVelocity() < 0.0f ? STATE_RAMP_SLIDE : STATE_RAMP_RUN_UP);
        return;
    }

    if (!GameCommon::isZero(prevForwardVelocity) && GameCommon::isZero(getForwardVelocity())) {
        hitWall();
    }

    if (hit & MEdgeMap::HIT_FLOOR) {
        const MathOrthoBox& bounds = floor->getBounds();
        if (bounds.m_max.y - 0.01f <= feetY) {
            groundPlayer();
        } else if (m_position.x < bounds.m_min.x) {
            if (m_position.x + m_shape->getLocalBounds().m_max.x * 0.2f >= bounds.m_min.x) {
                m_position.x = bounds.m_min.x + 0.02f;
                m_position.y = bounds.m_max.y;
            } else {
                m_velocity.x -= 1.0f;
                m_velocity.y = 0.1f;
            }
        } else if (m_position.x <= bounds.m_max.x) {
            groundPlayer();
        } else if (m_position.x + m_shape->getLocalBounds().m_min.x * 0.2f <= bounds.m_max.x) {
            m_position.x = bounds.m_max.x - 0.02f;
            m_position.y = bounds.m_max.y;
        } else {
            m_velocity.x += 1.0f;
            m_velocity.y = 0.1f;
        }
        m_material = floor->m_material;
    } else if (!GameCommon::isZero(m_velocity.y)) {
        fallPlayer();
    }

    if (hit & 8) {
        hitHead();
    }
}

void GameObjectPlayer::updateMovement(int dt) {
    AnimationState anim = m_animState;
    if (anim != ANIM_NONE && !m_animPlayer->isAnimating() && anim != ANIM_VAULT_JUMP) {
        animStateTransition(anim == ANIM_BOOST_JUMP ? ANIM_BOOST_JUMP_END : ANIM_NONE);
    }

    if (m_skipMovementOnce) {
        m_skipMovementOnce = false;
        return;
    }

    if (isInOriginAnimation()) {
        updateMovementOriginAnimation(getFacingDir(), dt);
    } else if (!m_positionInterpolation.isFinished()) {
        updateMovementLinearInterpolation(dt);
    } else if (m_lineMovement) {
        updateLineMovement(dt * 0.001f);
    } else if (m_noClipMovement) {
        updateMovementNoClip(dt * 0.001f);
    } else {
        updateMovementPhysics(dt * 0.001f);
    }

    float y = m_position.y;
    if (m_state == STATE_SWINGING) {
        y += 1.6f;
    }
    m_objectNode->setTranslation(m_position.x, y, m_position.z + m_zOffsetFilter.getFilteredValue());
    m_zOffsetFilter.update(dt);
}

void GameObjectPlayer::applyGestureDown() {
    switch (m_state) {
    case STATE_RUNNING:
        stateTransition(STATE_SLIDING);
        break;
    case STATE_FALLING_VERTICALLY:
    case STATE_FALLING_FORWARDS:
        m_landingRollTimer = 500;
        break;
    case STATE_WALL_CLIMB:
        stateTransition(STATE_WALL_SLIDE);
        clearGesture(GESTURE_DOWN);
        break;
    case STATE_HANG: {
        if (intersectsMap()) {
            return;
        }
        MathLine line(getForwardDirection(), 0.0f, 1.0f, 0.0f);
        line.m_origin *= getFacingDir() * 0.51f;
        line.m_origin += m_position;
        const MathOrthoBox& bounds = m_shape->getLocalBounds();
        float tMin = bounds.m_min.y;
        float tMax = bounds.m_max.y;
        m_map->intersects(line, 0, tMin, tMax);
        if (tMin >= 0.0f) {
            stateTransition(STATE_FALLING_VERTICALLY);
        } else {
            animStateTransition(ANIM_DROP_TO_WALL_SLIDE);
            stateTransition(STATE_WALL_SLIDE);
        }
        m_ledgeGrabBlock = 2;
        clearGesture(GESTURE_DOWN);
        break;
    }
    case STATE_WALL_SLIDE:
        stateTransition(STATE_FALLING_VERTICALLY);
        m_wallSlideBlock = 2;
        clearGesture(GESTURE_DOWN);
        break;
    case STATE_SWINGING:
        if (m_swingAmplitude == 0.0f) {
            stateTransition(STATE_FALLING_VERTICALLY);
        } else {
            setForwardVelocity(0.0f);
            stateTransition(STATE_FALLING_FORWARDS);
        }
        clearGesture(GESTURE_DOWN);
        break;
    case STATE_RAMP_RUN_UP:
        stateTransition(STATE_RAMP_SLIDE);
        break;
    case STATE_ZIPLINE:
        AppEngine::getAchievementData()->registerZiplineDrop();
        stateTransition(STATE_FALLING_FORWARDS);
        break;
    case STATE_FLYKICK:
        stateTransition(STATE_FALLING_FORWARDS);
        break;
    default:
        break;
    }
}

void GameObjectPlayer::applyGestureUp() {
    switch (m_state) {
    case STATE_IDLE:
        if (checkWallAction(getFacingDir(), false, 1)) {
            return;
        }
        if (checkJumpToZipLine()) {
            return;
        }
        if (checkLedgeScale()) {
            m_skipMovementOnce = true;
            return;
        }
        playerJump(false, true);
        break;
    case STATE_RUNNING:
        if (checkJumpToZipLine()) {
            return;
        }
        if (checkLedgeScale()) {
            m_skipMovementOnce = true;
            return;
        }
        if (checkWallrun()) {
            stateTransition(STATE_WALL_RUN);
            return;
        }
        if (m_clamberJumpWindow > 0) {
            playerJump(true, true);
        } else {
            playerJump(false, true);
        }
        break;
    case STATE_FALLING_FORWARDS:
    case STATE_FLYKICK:
        if (!checkWallrun()) {
            return;
        }
        if (m_wallRunCooldown > 0) {
            return;
        }
        stateTransition(STATE_WALL_RUN);
        break;
    case STATE_WALL_CLIMB: {
        int dir = -getFacingDir();
        setFacingDir((FacingDir)dir);
        setForwardVelocity(dir * 7.0f);
        setUpVelocity(5.0f);
        stateTransition(STATE_FALLING_FORWARDS);
        clearGesture(GESTURE_UP);
        break;
    }
    case STATE_CLAMBER:
        if (getVisualIndex() != m_clamberNextVisual) {
            return;
        }
        snapPlayerToGround();
        playerJump(true, true);
        clearGesture(GESTURE_UP);
        break;
    case STATE_HANG:
        completePositionInterpolation();
        stateTransition(STATE_HANG_CLAMBER);
        clearGesture(GESTURE_UP | GESTURE_JUMP);
        break;
    case STATE_SLIDING:
        stateTransition(STATE_RUNNING);
        break;
    case STATE_WALL_RUN:
    case STATE_RAMP_RUN_UP:
        playerJump(false, true);
        break;
    case STATE_WALL_SLIDE:
        animStateTransition(ANIM_WALL_KICK);
        stateTransition(STATE_FALLING_FORWARDS);
        setForwardVelocity(getFacingDir() * 7.0f);
        setUpVelocity(5.0f);
        break;
    case STATE_SWINGING:
        playerSwingLaunch();
        break;
    case STATE_RAMP_SLIDE:
        stateTransition(STATE_FALLING_FORWARDS);
        setForwardVelocity(getFacingDir() * 10.0f);
        setUpVelocity(5.0f);
        break;
    case STATE_LYING: {
        MathLine line(m_position, -(float)getFacingDir(), 0.0f, 0.0f);
        line.m_origin.y += m_shape->getLocalBounds().m_max.y;
        GameObject* obj = m_map->getGameObjectAt(line, this);
        if (obj != NULL && obj->isFlagSet(FLAG_POLICE)) {
            stateTransition(STATE_GETUP_MELEE);
            static_cast<GameObjectPolice*>(obj)->meleedFromPlayerJumpingFromProne();
        } else {
            stateTransition(STATE_GETTING_UP);
        }
        break;
    }
    default:
        break;
    }
}

void GameObjectPlayer::applyGestureJump() {
    switch (m_state) {
    case STATE_IDLE:
        if (checkWallAction(getFacingDir(), false, 1)) {
            clearGesture(GESTURE_UP);
            return;
        }
        playerJump(false, true);
        break;
    case STATE_RUNNING:
    case STATE_WALL_RUN:
    case STATE_RAMP_RUN_UP:
        playerJump(false, true);
        break;
    case STATE_WALL_CLIMB:
    case STATE_HANG: {
        int dir = -getFacingDir();
        setFacingDir((FacingDir)dir);
        setForwardVelocity(dir * 7.0f);
        setUpVelocity(5.0f);
        stateTransition(STATE_FALLING_FORWARDS);
        break;
    }
    case STATE_WALL_SLIDE:
        animStateTransition(ANIM_WALL_KICK);
        stateTransition(STATE_FALLING_FORWARDS);
        setForwardVelocity(getFacingDir() * 7.0f);
        setUpVelocity(5.0f);
        break;
    case STATE_SWINGING:
        playerSwingLaunch();
        break;
    case STATE_RAMP_SLIDE:
        stateTransition(STATE_FALLING_FORWARDS);
        setForwardVelocity(getFacingDir() * 10.0f);
        setUpVelocity(5.0f);
        break;
    default:
        break;
    }
}

void GameObjectPlayer::applyGestureStop() {
    if (m_state == STATE_RUNNING) {
        stateTransition(getForwardSpeed() < 7.0f ? STATE_IDLE : STATE_STOPPING);
    }
}

void GameObjectPlayer::applyGestureSide() {
    FacingDir oldDir = getFacingDir();
    switch (m_state) {
    case STATE_IDLE:
    case STATE_RUNNING:
    case STATE_BALANCE_RUNNING:
    case STATE_FALLING_VERTICALLY:
    case STATE_FALLING_FORWARDS:
    case STATE_HANG:
    case STATE_WALL_SLIDE:
    case STATE_SWINGING:
    case STATE_RAMP_RUN_UP:
        setFacingDir(isAnyGestureSet(GESTURE_ANY_LEFT) ? FACING_LEFT : FACING_RIGHT);
        break;
    default:
        break;
    }
    FacingDir dir = getFacingDir();

    switch (m_state) {
    case STATE_IDLE:
        if (!checkWallAction(dir, false, 1)) {
            stateTransition(STATE_RUNNING);
        }
        break;
    case STATE_RUNNING:
        if (dir == oldDir) {
            if (m_disarmCooldown > 0) {
                return;
            }
            m_disarmTimer = 1000;
            m_disarmCooldown = 2000;
        } else if (getForwardSpeed() < 7.0f) {
            stateTransition(STATE_RUNNING_TURNING);
            reverseFacingDir();
        } else {
            setForwardVelocity(dir * 5.0f);
        }
        break;
    case STATE_FALLING_VERTICALLY:
        setForwardVelocity((float)dir);
        stateTransition(STATE_FALLING_FORWARDS);
        break;
    case STATE_FALLING_FORWARDS:
        if (dir == oldDir) {
            stateTransition(STATE_FLYKICK);
        } else {
            setForwardVelocity(dir * midp::Math::min(midp::Math::abs(getForwardVelocity()), 1.0f));
        }
        break;
    case STATE_HANG:
        if (intersectsMap()) {
            if (dir != oldDir) {
                reverseFacingDir();
            }
            return;
        }
        if (dir == oldDir) {
            completePositionInterpolation();
            stateTransition(STATE_HANG_CLAMBER);
        } else {
            setForwardVelocity(getFacingDir() * 7.0f);
            setUpVelocity(5.0f);
            stateTransition(STATE_FALLING_FORWARDS);
        }
        break;
    case STATE_WALL_SLIDE:
        if (dir != oldDir) {
            reverseFacingDir();
            return;
        }
        stateTransition(STATE_FALLING_FORWARDS);
        setForwardVelocity(getFacingDir() * 0.01f);
        m_wallSlideBlock = 2;
        break;
    case STATE_SWINGING:
        if (dir == oldDir) {
            playerSwingLaunch();
        }
        break;
    case STATE_RAMP_RUN_UP:
        if (dir != oldDir) {
            stateTransition(STATE_RAMP_SLIDE);
        }
        break;
    default:
        break;
    }
}

void GameObjectPlayer::applyGestures() {
    if (m_crashed) {
        m_gestures = 0;
    }
    if (isAnyGestureSet(GESTURE_ANY_LEFT) != isAnyGestureSet(GESTURE_ANY_RIGHT)) {
        applyGestureSide();
    }
    if (isAnyGestureSet(GESTURE_STOP)) {
        applyGestureStop();
    }
    if (isAnyGestureSet(GESTURE_JUMP)) {
        applyGestureJump();
    }
    if (isAnyGestureSet(GESTURE_UP)) {
        applyGestureUp();
    }
    if (isAnyGestureSet(GESTURE_DOWN)) {
        applyGestureDown();
    }
}

void GameObjectPlayer::update(int dt) {
    if (m_state == STATE_NONE) {
        return;
    }
    if (m_disarmTimer > 0) {
        m_disarmTimer -= dt;
    }
    if (m_disarmCooldown > 0) {
        m_disarmCooldown -= dt;
    }
    if (m_clamberJumpWindow > 0) {
        m_clamberJumpWindow -= dt;
    }
    m_stateTime += dt;
    m_lastPosition = m_position;

    if (m_state == STATE_DEAD) {
        if (m_stateTime >= 1000) {
            playerFail();
        }
        return;
    }

    updateHealth(dt);
    applyGestures();
    setupAnimation();
    updateVelocity(dt);
    GameObjectRunner::update(dt);
    setupAnimation();
    updateMovement(dt);
    clearInvalidGestures();
    setupAnimation();
    updateState(dt);
    setupAnimation();
    updateSound(dt);
    if (m_ghostRecorder != NULL) {
        updateGhost(dt);
    }
    if (m_wallRunCooldown != -1) {
        m_wallRunCooldown -= dt;
    }
    if (m_ledgeGrabBlock == 1) {
        m_ledgeGrabBlock = 0;
    }
    if (m_wallSlideBlock == 1) {
        m_wallSlideBlock = 0;
    }

    if (isImmuneToOtherPlayer()) {
        if (m_state == STATE_MELEED_IN_THE_FACE || m_state == STATE_LYING) {
            return;
        }
        m_immunityTimer = midp::Math::max(0, m_immunityTimer - dt);
        m_objectNodeCull->setVisible((((m_immunityTimer + 255) >> 8) & 1) == 0);
    }
}

void GameObjectPlayer::animationStopped() {
    switch (m_state) {
    case STATE_START:
        stateTransition(STATE_IDLE);
        break;
    case STATE_CLAMBER:
        animStoppedStateClamber();
        break;
    case STATE_HANG_CLAMBER:
        animStoppedStateHangClamber();
        break;
    case STATE_SCALE_HAUL_UP:
        animStoppedStateScaleHaulUp();
        break;
    default:
        break;
    }
    refreshAnimation();
    setupAnimation();
}

void GameObjectPlayer::resetCheckpoint() {
    resetCommon();
    stateTransition(STATE_IDLE);
    m_position = m_checkpointPosition;
    setFacingDir(m_checkpointFacingDir);
}

void GameObjectPlayer::collidedWith(GameObject* other) {
    int type = other->getType();
    if (type == TYPE_SWING) {
        if (m_collisionCooldown != 0 && m_lastCollidedObject == other) {
            m_collisionCooldown = 2;
        } else if (m_state != STATE_SWINGING && getUpVelocity() > -18.0f) {
            m_position.x += other->m_position.x - m_position.x;
            m_position.y += other->m_position.y + (-1.6f - m_position.y);
            stateTransition(STATE_SWINGING);
            m_lastCollidedObject = other;
            m_collisionCooldown = 2;
        }
    } else if (type == TYPE_ZIPLINE) {
        if (m_state == STATE_ZIPLINE) {
            return;
        }
        GameObjectZipLine* zipLine = static_cast<GameObjectZipLine*>(other);
        MathLine line(zipLine->getZipLineLine());
        if ((line.m_direction.x > 0.0f) == (getFacingDir() == FACING_RIGHT) && getUpVelocity() > -18.0f) {
            float t = line.calculateTatX(m_position.x);
            if (t >= 0.0f && t <= 0.9f) {
                float lastLineY = line.calculateYatT(line.calculateTatX(m_lastPosition.x));
                float lineY = line.calculateYatT(t);
                if (lastLineY < m_lastPosition.y + 1.58f && lineY >= m_position.y + 1.58f) {
                    m_position.y = lineY - 1.59f;
                    playerZipLine(zipLine);
                }
            }
        }
    } else if (other->isFlagSet(FLAG_PLAYER)) {
        GameObjectPlayer* rival = static_cast<GameObjectPlayer*>(other);
        if (AppEngine::getLevelData()->isMultiplayerCombatAllowed() && isAttacking() &&
            !rival->isImmuneToOtherPlayer()) {
            if (rival->isAttacking()) {
                meleeByPlayer();
                rival->meleeByPlayer();
            } else {
                rival->meleeByPlayer();
                if (rival->getNumCollectablesCollected() != 0) {
                    SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
                    scene->getGameScreen(m_playerIndex)->incNumCollectablesCollected();
                    scene->getGameScreen(rival->getPlayerIndex())->decNumCollectablesCollected();
                }
            }
        }
    }
}

void GameObjectPlayer::resetLevel() {
    GameObjectRunner::resetLevel();
    setNoVelocity();
    m_numCollectables = 0;
    stateTransition(STATE_NONE);
    resetCommon();
    stateTransition(STATE_START);
    setFacingDir(m_startFacingDir);
    m_checkpointPosition = m_position;
    m_checkpointFacingDir = m_startFacingDir;
    m_animDirty = true;
    setVisual(VISUAL_SOLO, 1);
    if (m_ghostRecorder != NULL) {
        m_ghostRecorder->reset();
        m_ghostTime = 0;
    }
}

GameObjectPlayer::~GameObjectPlayer() {
    delete m_ghostRecorder;
}

GameObjectPlayer::GameObjectPlayer(MEdgeMap* map, int type, int playerIndex, float x, float y, float z,
                                   FacingDir facingDir)
    : GameObjectRunner(map, type, x, y, z),
      m_playerIndex(playerIndex),
      m_swingAmplitude(0),
      m_swingPhase(0),
      m_swingPhaseOffset(0),
      m_swingAngle(0),
      m_swingDir(1),
      m_lastCollidedObject(NULL),
      m_collisionCooldown(0),
      m_victorious(false),
      m_tiltNoiseFilter(SignalFilter::SMOOTHSTEP_FIR, 2500.0f, 0.0f),
      m_accelerometerNoiseTimer(0),
      m_zOffset(0),
      m_zOffsetFilter(SignalFilter::SMOOTHSTEP_FIR, 500.0f, 0.0f),
      m_startFacingDir(facingDir),
      m_checkpointPosition(x, y, z),
      m_checkpointFacingDir(facingDir),
      m_state(STATE_NONE),
      m_stateTime(0),
      m_disarmTimer(0),
      m_disarmCooldown(0),
      m_numCollectables(0),
      m_gestures(0),
      m_landingRollTimer(0),
      m_animState(ANIM_NONE),
      m_animSetupState(STATE_NONE),
      m_animDirty(false),
      m_ghostRecorder(NULL),
      m_ghostKeyframe(NULL),
      m_ghostTime(0),
      m_healthRegenDelay(0),
      m_soundState(0),
      m_soundAnimState(0),
      m_climbSoundSequence(-1),
      m_runSoundSequence(-1),
      m_crashed(false),
      m_maxSpeed(0),
      m_acceleration(0),
      m_runningTimer(0),
      m_positionInterpCollide(false),
      m_skipMovementOnce(false),
      m_noClipMovement(false),
      m_lineMovement(false),
      m_lineSpeed(0),
      m_lineMovementMode(0),
      m_immunityTimer(0),
      m_clamberVisual(-1),
      m_clamberNextVisual(-1),
      m_clamberExitSpeed(0),
      m_clamberOffsetHeight(0),
      m_ledgeGrabBlock(0),
      m_wallSlideBlock(0),
      m_jumpVariant(false),
      m_clamberJumpWindow(-1),
      m_rollIntoSlide(false),
      m_slideUnderObstacle(false),
      m_wallRunCooldown(-1),
      m_scaleVisual(0),
      m_scaleNextState(STATE_IDLE),
      m_scaleAnimState(ANIM_NONE),
      m_rampShape(NULL),
      m_zipLine(NULL),
      m_zipLineEndOffset(0),
      m_material(0),
      m_health(10.0f),
      m_achievementData(AppEngine::getCanvas()->getAchievementData()),
      m_lyingTimer(0) {
    if (playerIndex == 0) {
#ifdef MEDECOMP_PORT
        int levelModel = map->getModelId(1);
        int model = medecomp_player_model(levelModel);
        M3GAssets* assets = AppEngine::getM3GAssets();
        if (model < 0 || assets == NULL || model >= assets->m_models.m_length) model = levelModel;
        setVisualAssets(model, 0);
        medecomp_dress_player(m_objectNode);
#else
        setVisualAssets(map->getModelId(1), 0);
        M3GAssets::orphanNode(static_cast<m3g::Node*>(m_objectNode->find(103)));
        M3GAssets::orphanNode(static_cast<m3g::Node*>(m_objectNode->find(104)));
#endif
    } else {
        setVisualAssets(16, 0);
    }
    setFacingDir(facingDir);

    AnimationManager3D& animManager = AppEngine::getCanvas()->getAnimationManager3D();
    m_originNode = AppEngine::getM3GAssets()->loadNode(6);
    m_originAnimPlayer = new AnimPlayer3D(animManager);
    midp::INCREF(m_originAnimPlayer);
    m_originAnimPlayer->setNode(m_originNode);

    m_shapeNormal = new CollOctahedron(-0.5f, 0.0f, -0.5f, 0.5f, 1.8f, 0.5f);
    m_shapeNormal->setBounce(0.0f);
    m_shapeCrouch = new CollOctahedron(-0.5f, 0.0f, -0.5f, 0.5f, 0.5f, 0.5f);
    m_shapeCrouch->setBounce(0.0f);
    m_shapeBox = new CollOrthoHexahedron(-0.5f, 0.9f, -0.5f, 0.5f, 0.9f, 0.5f);
    m_shapeBox->setBounce(0.0f);
    m_shape = m_shapeNormal;

    m_objectNodeCull = new GameObjectCulledGraphic(m_objectNode, getGlobalBounds());
    m_map->addCullObject(m_objectNodeCull);
    setFacingDir(getFacingDir());
    stateTransition(STATE_START);

    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_SPEEDRUN
#ifdef MEDECOMP_PORT
        || AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_CHALLENGE
#endif
    ) {
        m_ghostRecorder = new GhostAnimationRecorder();
        int visual = hashVisual(VISUAL_SOLO, 1, getFacingDir());
        float blendWeight = get3WayAnimationBlendWeights();
        m_ghostKeyframe = new GhostKeyframe(m_ghostTime, m_position, visual, blendWeight);
        m_ghostRecorder->addKeyframe(m_ghostKeyframe);
        m_ghostTime = 0;
    }

    m_effectNode106 = static_cast<m3g::Node*>(m_objectNode->find(106));
    m_effectNode105 = static_cast<m3g::Node*>(m_objectNode->find(105));
    m_effectNode110 = static_cast<m3g::Node*>(m_objectNode->find(110));
}

GameObjectPlayer::PlayerState GameObjectPlayer::getState() {
    return m_state;
}

bool GameObjectPlayer::isAttacking() const {
    return m_state == STATE_ROLLING || m_state == STATE_SLIDING || m_state == STATE_FLYKICK;
}

bool GameObjectPlayer::isCrouchState(PlayerState state) {
    return state == STATE_ROLLING || state == STATE_SLIDING || state == STATE_LYING;
}

bool GameObjectPlayer::isAnyGestureSet(int gestures) const {
    return (m_gestures & gestures) != 0;
}

void GameObjectPlayer::clearGesture(int gesture) {
    m_gestures &= ~gesture;
}

void GameObjectPlayer::clearAllGestures() {
    m_gestures = 0;
}

void GameObjectPlayer::clearAllGesturesExcept(int gestures) {
    m_gestures &= gestures;
}

void GameObjectPlayer::animStateTransition(AnimationState state) {
    m_animState = state;
    m_animDirty = true;
}

void GameObjectPlayer::refreshAnimation() {
    m_animDirty = true;
}

float GameObjectPlayer::getForwardSpeed() const {
    return getForwardVelocity() * getFacingDir();
}

void GameObjectPlayer::setForwardSpeed(float speed) {
    setForwardVelocity(getFacingDir() * speed);
}

bool GameObjectPlayer::isImmuneToOtherPlayer() {
    return m_immunityTimer != 0;
}

bool GameObjectPlayer::isKicking() const {
    return m_state == STATE_FLYKICK;
}

bool GameObjectPlayer::isSliding() const {
    return m_state == STATE_SLIDING;
}

bool GameObjectPlayer::isMeleed() const {
    return m_state == STATE_MELEED_IN_THE_FACE;
}

bool GameObjectPlayer::isRolling() const {
    if (m_state != STATE_ROLLING) {
        return false;
    }
    return const_cast<GameObjectPlayer*>(this)->m_animPlayer->getAnimTime() < 500;
}

void GameObjectPlayer::setVictorious() {
    m_victorious = true;
}

bool GameObjectPlayer::isLying() const {
    return m_state == STATE_LYING;
}

bool GameObjectPlayer::isGetupMeleeing() const {
    return m_state == STATE_GETUP_MELEE;
}

bool GameObjectPlayer::isGettingUp() const {
    return m_state == STATE_GETTING_UP;
}

bool GameObjectPlayer::isDisarming() const {
    return m_disarmTimer > 0;
}

bool GameObjectPlayer::isProne() const {
    return m_state == STATE_LYING;
}

bool GameObjectPlayer::isOnRamp() const {
    return m_state == STATE_RAMP_RUN_UP || m_state == STATE_RAMP_SLIDE;
}

bool GameObjectPlayer::isDead() {
    return m_state == STATE_DYING || m_state == STATE_DEAD;
}

void GameObjectPlayer::decNumCollectablesCollected() {
    m_numCollectables--;
}

void GameObjectPlayer::addGesture(int gesture) {
    m_gestures |= gesture;
}

float GameObjectPlayer::getHealthFraction() {
    return fmax(0.0, m_health) * 0.1;
}

bool GameObjectPlayer::isVictorious() {
    return m_victorious;
}
