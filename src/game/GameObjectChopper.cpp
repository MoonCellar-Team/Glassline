#include "game/GameObjectChopper.h"

#include <math.h>

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectBullet.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectPlayer.h"
#include "game/M3GAssets.h"
#include "game/MathOrthoBox.h"
#include "game/MEdgeMap.h"
#include "game/Path.h"
#include "game/PathPoint.h"
#include "game/SceneGame.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "m3g/Node.h"
#include "m3g/Transform.h"
#include "runtime/midp/Math.h"

static const int MODEL_CHOPPER = 14;
static const int MODEL_FLAGS = 4;

static const int NODE_MAIN_ROTOR = 401;
static const int NODE_TAIL_ROTOR = 402;
static const int NODE_FLASH_RIGHT = 403;
static const int NODE_FLASH_LEFT = 404;

static const int SOUND_LOOP_0 = 50;
static const int SOUND_LOOP_1 = 51;

static const int SOUND_POOL_SHOT = 3;
static const int SOUND_GROUP_SHOT = 1;

static const int PATH_ACTION_SHOOT = 0;

static const float BULLET_SPEED = 40.0f;
static const int SHOT_INTERVAL = 150;
static const int BURST_TIME = 2000;
static const int BURST_PAUSE = 2000;
static const float SHOOT_RANGE_SQ = 400.0f;

static float maxf(float a, float b) {
    return a < b ? b : a;
}

const int GameObjectChopper::UPDATE_DIST_SQ = 625;

bool GameObjectChopper::shouldUpdate() {
    return m_distToPlayerSq <= UPDATE_DIST_SQ;
}

bool GameObjectChopper::playerInRange() {
    return (float)m_distToPlayerSq <= SHOOT_RANGE_SQ;
}

void GameObjectChopper::shootPlayer() {
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    MathVector from(m_position);
    MathVector to(player->m_position);
    from += CHOPPER_GUN_OFFSET;
    to += PLAYER_AIM_OFFSET;
    MathVector velocity = to - from;
    velocity.setLength(BULLET_SPEED);
    m_map->addObject(new GameObjectBullet(m_map, from.x, from.y, from.z, velocity));
    m_shotTimer = SHOT_INTERVAL;
    velocity.rotateYAxis(-m_yaw);
    (velocity.x > 0.0f ? m_flashRight : m_flashLeft)->setRenderingEnable(true);
    AppEngine* app = AppEngine::getCanvas();
    SoundManager& sound = app->getSoundManager();
    sound.playEventAt(
        app->getSceneGame()->getSoundPoolManager().getRandomSoundEventID(SOUND_POOL_SHOT, SOUND_GROUP_SHOT, -1),
        m_position.x, m_position.y, m_position.z, 1.0f);
}

void GameObjectChopper::updateSound(int dt) {
    const int sounds[2] = {SOUND_LOOP_0, SOUND_LOOP_1};
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    for (int i = 0; i < 2; i++) {
        if (m_loopSounds[i] == -1 || !sound.isHandlePlaying(m_loopSounds[i])) {
            m_loopSounds[i] = sound.playEvent(sounds[i], 1.0f);
            if (m_loopSounds[i] != -1) {
                sound.setLoopCountEvent(m_loopSounds[i], -1);
            }
        }
    }
    for (int i = 0; i < 2; i++) {
        if (m_loopSounds[i] != -1) {
            sound.setEventPosition(m_loopSounds[i], m_position.x, m_position.y, m_position.z);
        }
    }
}

void GameObjectChopper::updateOrientation(int dt) {
    MathVector ahead = m_spline.getPosition(m_splinePos + 5.0f);
    MathVector dir = ahead - m_position;
    float yaw = atan2(dir.z, dir.x) - 1.5707964f;
    while (yaw < 0.0f) {
        yaw = midp::Math::PI + midp::Math::PI + yaw;
    }
    while (midp::Math::PI + midp::Math::PI < yaw) {
        yaw = midp::Math::PI * -2.0f + yaw;
    }
    float seconds = dt * 0.001f;
    float pi = midp::Math::PI;
    m_rollFilter.setTargetValue((yaw - m_yaw) / seconds * 0.5f);
    m_rollFilter.update(dt);
    float roll = m_rollFilter.getFilteredValue();
    m_yaw = yaw;

    float speed = m_speedFilter.getFilteredValue();
    float pitchTarget = (speed - m_prevSpeed) / seconds + speed;
    m_pitchFilter.setTargetValue(pitchTarget + pitchTarget);
    m_pitchFilter.update(dt);
    float pitch = m_pitchFilter.getFilteredValue();
    m_prevSpeed = speed;

    float toDegrees = 180.0f / pi;
    m3g::Transform transform;
    transform.postRotate(yaw * toDegrees, 0.0f, -1.0f, 0.0f);
    transform.postRotate(roll * toDegrees, 0.0f, 0.0f, 1.0f);
    transform.postRotate(pitch, 1.0f, 0.0f, 0.0f);
    m_objectNode->setTransform(&transform);

    m_rotorAngle += seconds + seconds;
    if (m_rotorAngle >= pi + pi) {
        m_rotorAngle += pi * -2.0f;
    }
    m_mainRotor->setOrientation(m_rotorAngle * toDegrees, 0.0f, 1.0f, 0.0f);
    m_tailRotor->setOrientation(-(m_rotorAngle * toDegrees), 1.0f, 0.0f, 0.0f);
    m_rotorFrame = !m_rotorFrame;
    m_mainRotor->setRenderingEnable(m_rotorFrame);
    m_tailRotor->setRenderingEnable(m_rotorFrame);
}

bool GameObjectChopper::shouldShoot(int dt) {
    if (m_path == NULL) {
        return false;
    }
    if (m_burstTime <= 0) {
        m_burstPause -= dt;
        if (m_burstPause <= 0) {
            m_burstTime = BURST_TIME;
        }
    }
    if (m_burstPause > 0 && m_burstTime <= 0) {
        return false;
    }
    m_burstPause = BURST_PAUSE;
    if (!playerInRange()) {
        m_burstTime = 0;
        m_burstPause = 0;
    }
    if (m_path->getPoint(m_spline.getNodeAtLength(m_splinePos))->m_action != PATH_ACTION_SHOOT) {
        return false;
    }
    if (m_burstTime <= 0) {
        return false;
    }
    m_burstTime -= dt;
    m_shotTimer -= dt;
    return m_shotTimer <= 0;
}

void GameObjectChopper::updateMovement(int dt) {
    float seconds = dt * 0.001f;
    const PathPoint* point = m_path->getPoint(m_spline.getNodeAtLength(m_splinePos));
    m_speedFilter.setTargetValue((float)point->m_speed);
    m_speedFilter.update(dt);
    float speed = m_speedFilter.getFilteredValue();

    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    MathVector playerPos(player->m_position);
    MathVector playerDir = player->getForwardDirection() * (float)player->getFacingDir();
    MathVector ahead = m_spline.getPosition(m_splinePos + 0.001f);
    ahead.normalise();
    MathVector flightDir(ahead);

    float factor;
    if (playerDir.x * flightDir.x > 0.0f) {
        MathVector fromPlayer = m_position - playerPos;
        fromPlayer.z = 0.0f;
        float distSq = fromPlayer.getLengthSq();
        if (distSq > 25.0f) {
            float over = sqrtf(distSq) - 5.0f;
            if (fromPlayer.x * flightDir.x > 0.0f) {
                factor = maxf(0.0f, over * -0.05f + 1.0f);
            } else {
                factor = maxf(0.0f, over * 0.05f + 1.0f);
            }
        } else {
            factor = 1.0f;
        }
    } else {
        factor = 1.0f;
    }

    m_splinePos += speed * factor * seconds;
    if (m_splinePos >= m_splineLength) {
        m_splinePos -= m_splineLength;
    }
    m_position = m_spline.getPosition(m_splinePos);
    float t = m_time * 0.001f;
    m_hover.x = cosf(t) * 0.5f;
    m_hover.y = sinf(t + t) * 0.25f;
    m_position += m_hover;
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
}

void GameObjectChopper::update(int dt) {
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    m_distToPlayerSq = (int)getDistanceToSq(player);
    if (m_objectNodeCull != NULL) {
        m_objectNodeCull->getBounds().setCoordinates(getGlobalBounds());
    }
    if (!shouldUpdate()) {
        return;
    }
    m_time += dt;
    if (m_path != NULL) {
        updateMovement(dt);
        updateOrientation(dt);
    }
    updateSound(dt);
    m_flashRight->setRenderingEnable(false);
    m_flashLeft->setRenderingEnable(false);
    if (shouldShoot(dt)) {
        shootPlayer();
    }
}

void GameObjectChopper::resetLevel() {
    m_position = m_startPosition;
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    for (int i = 0; i < 2; i++) {
        if (m_loopSounds[i] != -1) {
            sound.stopEvent(m_loopSounds[i]);
            m_loopSounds[i] = -1;
        }
    }
    m_splinePos = 0.0f;
    m_prevSpeed = 0.0f;
    m_rollFilter.setSteadyState(0.0f);
    m_pitchFilter.setSteadyState(0.0f);
    m_speedFilter.setSteadyState(0.0f);
    m_shotTimer = 0;
    m_burstTime = 0;
    m_rotorFrame = true;
    m_yaw = 0.0f;
    m_time = 0;
}

GameObjectChopper::~GameObjectChopper() {
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    for (int i = 0; i < 2; i++) {
        if (m_loopSounds[i] != -1) {
            sound.stopEvent(m_loopSounds[i]);
            m_loopSounds[i] = -1;
        }
    }
    m_model = midp::ReferenceCountedPointer<m3g::Node>(NULL);
}

void GameObjectChopper::setupSpline() {
    m_spline.addNode(m_startPosition);
    int count = m_path->getPointCount();
    for (int i = 0; i < count; i++) {
        m_spline.addNode(MathVector(*m_path->getPoint(i)));
    }
    m_spline.buildSpline();
    m_splineLength = m_spline.getMaxLength();
}

void GameObjectChopper::setModel() {
    AppEngine* app = AppEngine::getCanvas();
    M3GAssets* assets = AppEngine::getM3GAssets();
    m3g::Group* world = app->getSceneGame()->getM3GWorld();
    m_model = assets->loadModel(MODEL_CHOPPER, MODEL_FLAGS);
    m3g::Group* group = new m3g::Group();
    group->addChild(m_model);
    world->addChild(group);
    m_objectNode = group;
    m_mainRotor = static_cast<m3g::Node*>(m_model->find(NODE_MAIN_ROTOR));
    m_tailRotor = static_cast<m3g::Node*>(m_model->find(NODE_TAIL_ROTOR));
    m_flashRight = static_cast<m3g::Node*>(m_model->find(NODE_FLASH_RIGHT));
    m_flashLeft = static_cast<m3g::Node*>(m_model->find(NODE_FLASH_LEFT));
    for (int i = 0; i < 4; i++) {
        m_unusedNodes[i] = static_cast<m3g::Node*>(m_model->find(NODE_FLASH_LEFT + 1 + i));
    }
    m_flashRight->setRenderingEnable(false);
    m_flashLeft->setRenderingEnable(false);
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
}

GameObjectChopper::GameObjectChopper(MEdgeMap* map, float x, float y, float z, int pathIndex)
    : GameObject(map, TYPE, x, y, z),
      m_model(NULL),
      m_unk80(0),
      m_mainRotor(NULL),
      m_tailRotor(NULL),
      m_flashRight(NULL),
      m_flashLeft(NULL),
      m_unusedNodes(),
      m_rotorAngle(0.0f),
      m_distToPlayerSq(0),
      m_path(NULL),
      m_time(0),
      m_splinePos(0.0f),
      m_splineLength(0.0f),
      m_prevSpeed(0.0f),
      m_yaw(0.0f),
      m_rollFilter(SignalFilter::SMOOTHSTEP_FIR, 1000.0f, 0.0f),
      m_pitchFilter(SignalFilter::SMOOTHSTEP_FIR, 1000.0f, 0.0f),
      m_speedFilter(SignalFilter::SMOOTHSTEP_FIR, 1000.0f, 0.0f),
      m_burstTime(0),
      m_burstPause(BURST_PAUSE),
      m_shotTimer(0),
      m_rotorFrame(true),
      m_unk94c(1000) {
    m_loopSounds[0] = -1;
    m_loopSounds[1] = -1;
    setModel();
    m_shape = new CollOrthoHexahedron(-9.0f, -1.0f, -6.0f, 9.0f, 6.0f, 6.0f);
    m_objectNodeCull = new GameObjectCulledGraphic(m_objectNode, getGlobalBounds());
    m_map->addCullObject(m_objectNodeCull);
    if (pathIndex != -1) {
        m_path = m_map->getPath(pathIndex);
        setupSpline();
    }
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    if (!sound.isEventLoaded(SOUND_LOOP_0)) {
        sound.loadEvent(SOUND_LOOP_0);
    }
    if (!sound.isEventLoaded(SOUND_LOOP_1)) {
        sound.loadEvent(SOUND_LOOP_1);
    }
}
