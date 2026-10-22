#include "game/GameObjectPolice.h"

#include <math.h>
#include <stdlib.h>

#include "ea_online/SpywareManager.h"
#include "game/AchievementData.h"
#include "game/AnimPlayer3D.h"
#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectBullet.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectPlayer.h"
#include "game/MathLine.h"
#include "game/MEdgeMap.h"
#include "game/Path.h"
#include "game/PathPoint.h"
#include "game/SceneGame.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "game/SoundSequencer.h"
#include "m3g/Node.h"
#ifdef MEDECOMP_PORT
#include "port/net.h"
#endif

static const int FLAG_PLAYER = 1;

static const int LOOP = AnimPlayer3D::START_LOOP;
static const int RESTART = AnimPlayer3D::START_RESTART;

static const int SOUND_POOL_GUNSHOT = 3;
static const int SOUND_POOL_HIT = 5;
static const int SOUND_GROUP_COP_HIT = 0;
static const int SOUND_GROUP_HIT = 1;

static const int SOUND_FALL_SCREAM = 80;

static const int SEQUENCE_DISARM = 16;

static const float KNOCKDOWN_RANGE_SQ = 0.8f * 0.8f;
static const float MELEE_RANGE_SQ = 1.273f * 1.273f;

static const float RAND_TO_PERCENT = 100.0f / 2147483648.0f;

const int GameObjectPolice::testAnims[NUM_TEST_ANIMS] = {92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106};

const int GameObjectPolice::testChannels[NUM_TEST_ANIMS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

void GameObjectPolice::meleedFromPlayerJumpingFromProne() {
    brainStateTransition(BRAIN_MELEED_FROM_PRONE);
}

bool GameObjectPolice::isActive() {
    return m_brainState < BRAIN_KNOCKED_DOWN || m_brainState > BRAIN_FALLEN;
}

bool GameObjectPolice::facingPlayer() {
    if (m_player->m_position.x < m_position.x) {
        return m_facing < midp::Math::PI;
    }
    return m_facing > midp::Math::PI;
}

bool GameObjectPolice::playerInRange() {
    int range = facingPlayer() ? m_tuning.sightRangeAhead : m_tuning.sightRangeBehind;
    return m_distToPlayerSq < range * range;
}

static float TURN_SPEED = midp::Math::PI * 8.0f;

void GameObjectPolice::facePlayer() {
    setFacingDir(m_player->m_position.x >= m_position.x ? FACING_RIGHT : FACING_LEFT);
}

void GameObjectPolice::shoot() {
    MathVector velocity;
    velocity.z += 12.0f;
    velocity.rotateYAxis(m_facing);
    float height = m_state == STATE_STAND ? 1.6f : 1.2f;
    GameObject* bullet = NULL;
#ifdef MEDECOMP_PORT
    bullet = port::net::enemyBullet(m_map, m_position.x, m_position.y + height, m_position.z, velocity);
#endif
    m_map->addObject(bullet != NULL ? bullet
                                    : new GameObjectBullet(m_map, m_position.x, m_position.y + height, m_position.z, velocity));
    AppEngine* app = AppEngine::getCanvas();
    SoundManager& sound = app->getSoundManager();
    sound.playEventAt(app->getSceneGame()->getSoundPoolManager().getRandomSoundEventID(SOUND_POOL_GUNSHOT, 0, -1),
                      m_position.x, m_position.y, m_position.z, 1.0f);
}

bool GameObjectPolice::sightedPlayer() {
    bool facing = facingPlayer();
    int distSq = m_distToPlayerSq;
    bool inRange = playerInRange();
    bool los = checkLOSWithPlayer();
    GameObjectPlayer* player = m_player;
    bool dead = player->isDead();
    bool sliding = player->isSliding();
    bool kicking = player->isKicking();
    bool rolling = player->isRolling();
    if (distSq <= 3 && kicking) {
        return true;
    }
    if (!los || !inRange || dead) {
        return false;
    }
    if (facing) {
        return true;
    }
    return !kicking && !sliding && !rolling;
}

bool GameObjectPolice::shouldTryShot(int dt) {
    m_shotTimer -= dt;
    if (m_shotTimer > 0) {
        return false;
    }
    m_shotCheckTimer -= dt;
    if (m_shotCheckTimer > 0) {
        return false;
    }
    m_shotCheckTimer += m_tuning.shotCheckInterval;
    if ((float)m_tuning.shotChance > rand() * RAND_TO_PERCENT) {
        m_shotTimer += m_tuning.shotDelay;
        return true;
    }
    return false;
}

bool GameObjectPolice::shouldSwitchPosture(int dt) {
    m_postureTimer -= dt;
    if (m_postureTimer > 0) {
        return false;
    }
    m_postureCheckTimer -= dt;
    if (m_postureCheckTimer > 0) {
        return false;
    }
    m_postureCheckTimer += m_tuning.postureCheckInterval;
    if ((float)m_tuning.postureChance > rand() * RAND_TO_PERCENT) {
        m_postureTimer += m_tuning.postureDelay;
        return true;
    }
    return false;
}

void GameObjectPolice::switchPosture() {
    if (m_state == STATE_STAND) {
        if (getDistanceToSq(AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject()) < 16.0f) {
            return;
        }
        stateTransition(STATE_CROUCH);
    } else {
        stateTransition(STATE_STAND);
    }
}

void GameObjectPolice::updateTestAnims(int dt) {
    m_testAnimTimer -= dt;
    if (m_testAnimTimer > 0) {
        return;
    }
    m_testAnimIndex = (m_testAnimIndex + 1) % NUM_TEST_ANIMS;
    startBlendedAnim(testAnims[m_testAnimIndex], testChannels[m_testAnimIndex], LOOP);
    m_testAnimTimer = 5000;
}

void GameObjectPolice::updateState(int dt) {
    if (m_state == NPC_STATE_TEST_ANIMS) {
        updateTestAnims(dt);
    } else if (m_state == NPC_STATE_START) {
        stateTransition(NPC_STATE_ACTIVE);
    }
}

void GameObjectPolice::playRandomGruntSound() {
    AppEngine::getCanvas()->rand(0, 100);
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    int r = AppEngine::getCanvas()->rand(0, 100);
    int id;
    if (r < 20) {
        id = 85;
    } else if (r < 40) {
        id = 86;
    } else if (r < 60) {
        id = 87;
    } else if (r < 80) {
        id = 88;
    } else {
        id = 89;
    }
    sound.playEventAt(id, m_position.x, m_position.y, m_position.z, 1.0f);
}

void GameObjectPolice::animStateTransition(AnimState state) {
    switch (state) {
    case ANIM_IDLE:
        if (m_animState == ANIM_ALERT) {
            startBlendedAnim(99, 7, RESTART);
        } else {
            startBlendedAnim(92, 0, LOOP);
        }
        break;
    case ANIM_PATROL:
        startBlendedAnim(118, 26, RESTART);
        break;
    case ANIM_UNUSED:
        startBlendedAnim(117, 25, RESTART);
        break;
    case ANIM_ALERT:
        if (m_animState != ANIM_IDLE) {
            animStateTransition(m_state == STATE_STAND ? ANIM_ALERT_STAND : ANIM_ALERT_CROUCH);
            return;
        }
        startBlendedAnim(93, 1, RESTART);
        break;
    case ANIM_ALERT_STAND:
        startBlendedAnim(94, 2, RESTART);
        break;
    case ANIM_AIM_STAND:
        if (m_animState == ANIM_FIRE_STAND) {
            startBlendedAnim(97, 5, LOOP);
        } else {
            startBlendedAnim(96, 4, RESTART);
        }
        break;
    case ANIM_FIRE_STAND:
        startBlendedAnim(98, 6, RESTART);
        break;
    case ANIM_ALERT_CROUCH:
        startBlendedAnim(101, 9, RESTART);
        break;
    case ANIM_AIM_CROUCH:
        if (m_animState == ANIM_FIRE_CROUCH) {
            startBlendedAnim(104, 12, LOOP);
        } else {
            startBlendedAnim(103, 11, RESTART);
        }
        break;
    case ANIM_FIRE_CROUCH:
        startBlendedAnim(105, 13, RESTART);
        break;
    case ANIM_MELEE:
        startBlendedAnim(119, 28, RESTART);
        break;
    case ANIM_DISARMED:
        startBlendedAnim(120, 27, RESTART);
        break;
    case ANIM_KNOCKED_FORWARD:
        startBlendedAnim(107, 15, RESTART);
        break;
    case ANIM_KNOCKED_FORWARD_HIGH:
        startBlendedAnim(109, 17, RESTART);
        break;
    case ANIM_KNOCKED_BACKWARD:
        startBlendedAnim(110, 18, RESTART);
        break;
    case ANIM_KNOCKED_BACKWARD_HIGH:
        startBlendedAnim(112, 20, RESTART);
        break;
    case ANIM_GET_UP_FORWARD:
        startBlendedAnim(115, 23, RESTART);
        break;
    case ANIM_GET_UP_BACKWARD:
        startBlendedAnim(116, 24, RESTART);
        break;
    case ANIM_MELEED_FROM_PRONE:
        startBlendedAnim(121, 29, RESTART);
        break;
    case ANIM_FALL_FORWARD:
    case ANIM_FALL_BACKWARD: {
        if (state == ANIM_FALL_FORWARD) {
            startBlendedAnim(122, 32, LOOP);
        } else {
            startBlendedAnim(123, 30, LOOP);
        }
        SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
        int r = AppEngine::getCanvas()->rand(0, 99);
        int id;
        if (r < 20) {
            id = 90;
        } else if (r < 40) {
            id = 91;
        } else if (r < 60) {
            id = 92;
        } else if (r < 80) {
            id = 93;
        } else {
            id = SOUND_FALL_SCREAM;
        }
        sound.playEventAt(id, m_position.x, m_position.y, m_position.z, 1.0f);
        break;
    }
    case ANIM_LAND: {
        MathLine down(m_position.x, m_position.y, m_position.z, 0.0f, -1.0f, 0.0f);
        CollShape* below = m_map->getAttributeShapeAt(down, 0);
        if (below != NULL && below->isRamp()) {
            int type = below->getShapeType();
            m_objectNode->setOrientation(type == 2 ? 30.0f : -30.0f, 0.0f, 0.0f, 1.0f);
            float tMin = 0.0f;
            float tMax = 2.0f;
            m_map->intersects(down, 0, tMin, tMax);
            m_modelOffsetY = -tMin;
        }
        if (m_animState == ANIM_FALL_FORWARD) {
            startBlendedAnim(124, 33, RESTART);
        } else {
            startBlendedAnim(125, 31, RESTART);
        }
        break;
    }
    default:
        break;
    }
    m_animState = state;
}

void GameObjectPolice::updateAnimState(int dt) {
    switch (m_animState) {
    case ANIM_IDLE:
        if (!m_animPlayer->isAnimating()) {
            startBlendedAnim(92, 0, LOOP);
        }
        break;
    case ANIM_ALERT:
        if (!m_animPlayer->isAnimating()) {
            animStateTransition(ANIM_ALERT_STAND);
        }
        break;
    case ANIM_ALERT_STAND:
        if (!m_animPlayer->isAnimating()) {
            startBlendedAnim(95, 3, LOOP);
        }
        break;
    case ANIM_AIM_STAND:
        if (!m_animPlayer->isAnimating()) {
            startBlendedAnim(97, 5, LOOP);
        }
        break;
    case ANIM_FIRE_STAND:
        if (!m_animPlayer->isAnimating()) {
            animStateTransition(ANIM_AIM_STAND);
        }
        break;
    case ANIM_ALERT_CROUCH:
        if (!m_animPlayer->isAnimating()) {
            startBlendedAnim(102, 10, LOOP);
        }
        break;
    case ANIM_AIM_CROUCH:
        if (!m_animPlayer->isAnimating()) {
            startBlendedAnim(104, 12, LOOP);
        }
        break;
    case ANIM_FIRE_CROUCH:
        if (!m_animPlayer->isAnimating()) {
            animStateTransition(ANIM_AIM_CROUCH);
        }
        break;
    case ANIM_MELEE:
        if (!m_animPlayer->isAnimating()) {
            animStateTransition(ANIM_ALERT);
        }
        break;
    case ANIM_KNOCKED_FORWARD:
        if (!m_animPlayer->isAnimating()) {
            startBlendedAnim(113, 21, LOOP);
        }
        break;
    case ANIM_KNOCKED_BACKWARD:
        if (!m_animPlayer->isAnimating()) {
            startBlendedAnim(114, 22, LOOP);
        }
        break;
    case ANIM_LAND:
        if (!m_animPlayer->isAnimating()) {
            stateTransition(NPC_STATE_DISABLED);
        }
        break;
    default:
        break;
    }
}

void GameObjectPolice::stateTransition(NPCState state) {
    if (m_state == state) {
        return;
    }
    switch (state) {
    case STATE_STAND:
        animStateTransition(m_state == NPC_STATE_START ? ANIM_IDLE : ANIM_ALERT_STAND);
        break;
    case STATE_CROUCH:
        animStateTransition(ANIM_ALERT_CROUCH);
        break;
    case STATE_PATROL:
        animStateTransition(ANIM_PATROL);
        break;
    case STATE_UNUSED:
        animStateTransition(ANIM_UNUSED);
        break;
    default:
        break;
    }
    m_state = state;
    m_stateTime = 0;
}

void GameObjectPolice::brainStateTransition(BrainState state) {
    m_facePlayer = false;
    switch (state) {
    case BRAIN_IDLE:
        animStateTransition(ANIM_IDLE);
        break;
    case BRAIN_PATROL:
        m_path = m_map->getPath(m_pathIndex);
        m_pathNode = 0;
        stateTransition(STATE_PATROL);
        break;
    case BRAIN_ALERT:
        if (AppEngine::getCanvas()->rand(0, 100) < 20) {
            SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
            int r = AppEngine::getCanvas()->rand(0, 100);
            int id;
            if (r < 25) {
                id = 81;
            } else if (r < 50) {
                id = 82;
            } else if (r < 75) {
                id = 83;
            } else {
                id = 84;
            }
            sound.playEventAt(id, m_position.x, m_position.y, m_position.z, 1.0f);
        }
        setNoVelocity();
        facePlayer();
        m_shotTimer = m_tuning.shotDelay;
        m_shotCheckTimer = m_tuning.shotCheckInterval;
        animStateTransition(ANIM_ALERT);
        m_facePlayer = true;
        break;
    case BRAIN_AIM:
        facePlayer();
        animStateTransition(m_state == STATE_STAND ? ANIM_AIM_STAND : ANIM_AIM_CROUCH);
        m_aimTimer = m_tuning.aimTime;
        m_facePlayer = true;
        break;
    case BRAIN_FIRE:
        animStateTransition(m_state == STATE_STAND ? ANIM_FIRE_STAND : ANIM_FIRE_CROUCH);
        shoot();
        m_fireTimer = m_tuning.fireInterval;
        m_facePlayer = true;
        break;
    case BRAIN_MELEE:
        animStateTransition(ANIM_MELEE);
        break;
    case BRAIN_KNOCKED_DOWN: {
        m_stateTime = 0;
        m_knockdownPos = m_position;
        setNoVelocity();
        AppEngine* app = AppEngine::getCanvas();
        SoundEventPoolManager& pools = app->getSceneGame()->getSoundPoolManager();
        SoundManager& sound = app->getSoundManager();
        AppEngine::getAchievementData()->registerKill(app->getSceneGame()->getRaceTime() >> 10);
        if (m_player->isSliding() || m_player->isRolling()) {
            animStateTransition(facingPlayer() ? ANIM_KNOCKED_FORWARD : ANIM_KNOCKED_BACKWARD);
            sound.playEvent(pools.getRandomSoundEventID(SOUND_POOL_HIT, SOUND_GROUP_HIT, -1), 1.0f);
            playRandomGruntSound();
            break;
        }
        MathVector toPlayer = m_player->m_position - m_position;
        AnimState anim;
        if (facingPlayer()) {
            if (toPlayer.y < 0.5f) {
                anim = ANIM_KNOCKED_FORWARD;
            } else if (toPlayer.y < 1.0f) {
                anim = ANIM_KNOCKED_BACKWARD;
            } else {
                anim = ANIM_KNOCKED_BACKWARD_HIGH;
            }
        } else {
            if (toPlayer.y < 0.5f) {
                anim = ANIM_KNOCKED_BACKWARD;
            } else if (toPlayer.y < 1.0f) {
                anim = ANIM_KNOCKED_FORWARD;
            } else {
                anim = ANIM_KNOCKED_FORWARD_HIGH;
            }
        }
        animStateTransition(anim);
        m_velocity.x = m_player->getVelocity().x * 1.3f;
        m_deceleration.x = 9.0f;
        sound.playEvent(pools.getRandomSoundEventID(SOUND_POOL_HIT, SOUND_GROUP_HIT, -1), 1.0f);
        playRandomGruntSound();
        break;
    }
    case BRAIN_GET_UP:
        animStateTransition(m_animState == ANIM_KNOCKED_FORWARD ? ANIM_GET_UP_FORWARD : ANIM_GET_UP_BACKWARD);
        break;
    case BRAIN_DISARMED:
        animStateTransition(ANIM_DISARMED);
        AppEngine::getCanvas()->getSceneGame()->getSoundSequencer().playSequence(SEQUENCE_DISARM, NULL);
        break;
    case BRAIN_MELEED_FROM_PRONE:
        m_stateTime = 0;
        m_knockdownPos = m_position;
        setNoVelocity();
        animStateTransition(ANIM_MELEED_FROM_PRONE);
        break;
    default:
        break;
    }
    m_brainState = state;
#ifdef MEDECOMP_PORT
    port::net::enemyChanged(this);
#endif
}

void GameObjectPolice::patrolBrainUpdate(int dt) {
    MathVector toNode(*m_path->getPoint(m_pathNode));
    toNode -= m_position;
    toNode.y = 0;
    if (toNode.getLength() <= 0.4f) {
        m_pathNode = (m_pathNode + 1) % m_path->getPointCount();
        toNode = *m_path->getPoint(m_pathNode);
        toNode -= m_position;
    }
    toNode.y = 0;
    toNode.setLength(1.0f);
    m_velocity.x = toNode.x;
    m_velocity.z = toNode.z;
    m_targetFacing = facingFromDirection(m_velocity.x, m_velocity.z);
    if (sightedPlayer()) {
        stateTransition(STATE_STAND);
        brainStateTransition(BRAIN_AIM);
        setNoVelocity();
    }
}

void GameObjectPolice::fireBrainUpdate(int dt) {
    m_fireTimer -= dt;
    if (m_fireTimer > 0) {
        return;
    }
    if ((float)m_tuning.burstChance > rand() * RAND_TO_PERCENT) {
        animStateTransition(m_state == STATE_STAND ? ANIM_FIRE_STAND : ANIM_FIRE_CROUCH);
        shoot();
        m_fireTimer = m_tuning.fireInterval;
    } else {
        brainStateTransition(BRAIN_ALERT);
    }
}

void GameObjectPolice::aimBrainUpdate(int dt) {
    GameObjectPlayer* player = m_player;
    if (player->isDead()) {
        brainStateTransition(BRAIN_IDLE);
        return;
    }
    if (player->isLying() || !checkLOSWithPlayer()) {
        return;
    }
    if (fabsf(m_player->m_position.y - m_position.y) > 2.0f) {
        return;
    }
    m_aimTimer -= dt;
    if (m_aimTimer > 0) {
        return;
    }
    brainStateTransition(BRAIN_FIRE);
}

void GameObjectPolice::alertBrainUpdate(int dt) {
    if (shouldSwitchPosture(dt)) {
        switchPosture();
        return;
    }
    if (shouldTryShot(dt)) {
        brainStateTransition(BRAIN_AIM);
    }
}

void GameObjectPolice::updateBrain(int dt) {
    switch (m_brainState) {
    case BRAIN_IDLE:
        if (sightedPlayer()) {
            brainStateTransition(BRAIN_AIM);
        }
        break;
    case BRAIN_PATROL:
        patrolBrainUpdate(dt);
        break;
    case BRAIN_ALERT:
        alertBrainUpdate(dt);
        break;
    case BRAIN_AIM:
        aimBrainUpdate(dt);
        break;
    case BRAIN_FIRE:
        fireBrainUpdate(dt);
        break;
    case BRAIN_MELEE: {
        int prev = m_stateTime;
        m_stateTime += dt;
        if (prev < 280 && m_stateTime >= 280) {
            AppEngine* app = AppEngine::getCanvas();
            SoundManager& sound = app->getSoundManager();
            sound.playEvent(
                app->getSceneGame()->getSoundPoolManager().getRandomSoundEventID(SOUND_POOL_HIT, SOUND_GROUP_COP_HIT, -1),
                1.0f);
        }
    }
    case BRAIN_GET_UP:
        if (!m_animPlayer->isAnimating()) {
            brainStateTransition(BRAIN_ALERT);
        }
        break;
    case BRAIN_KNOCKED_DOWN:
        if (m_position.y - m_knockdownPos.y < -2.0f) {
            brainStateTransition(BRAIN_FALLEN);
            animStateTransition(m_animState == ANIM_KNOCKED_FORWARD || m_animState == ANIM_KNOCKED_FORWARD_HIGH
                                    ? ANIM_FALL_FORWARD
                                    : ANIM_FALL_BACKWARD);
            AppEngine::getAchievementData()->registerEnemyFall();
            break;
        }
        m_stateTime += dt;
        if (m_stateTime >= 5000) {
            brainStateTransition(BRAIN_GET_UP);
        }
        break;
    case BRAIN_MELEED_FROM_PRONE: {
        AppEngine* app = AppEngine::getCanvas();
        SceneGame* scene = app->getSceneGame();
        int prev = m_stateTime;
        m_stateTime += dt;
        if (prev < 650 && m_stateTime >= 650) {
            SoundManager& sound = app->getSoundManager();
            sound.playEvent(scene->getSoundPoolManager().getRandomSoundEventID(SOUND_POOL_HIT, SOUND_GROUP_HIT, -1),
                            1.0f);
            playRandomGruntSound();
        }
        if (m_stateTime < 650 && !scene->getSinglePlayerObject()->isGetupMeleeing()) {
            brainStateTransition(BRAIN_ALERT);
        }
        if (!m_animPlayer->isAnimating()) {
            brainStateTransition(BRAIN_GET_UP);
        }
        break;
    }
    case BRAIN_FALLEN:
        m_fallenTime += dt;
        break;
    default:
        break;
    }
    if (m_facePlayer) {
        updateFacePlayer(dt);
    }
}

void GameObjectPolice::updatePosition(int dt) {
    CollShape* hit = NULL;
    MathVector normal;
    int flags = m_map->moveObjectPhysics(m_position, m_velocity, dt * 0.001f, m_shape, &hit, normal);
    if (m_brainState == BRAIN_PATROL) {
        m_deceleration.x = 0;
    } else {
        m_velocity.z = 0;
        m_deceleration.x = 9.0f;
    }

    if ((m_animState == ANIM_FALL_FORWARD || m_animState == ANIM_FALL_BACKWARD) && (flags & MEdgeMap::HIT_FLOOR)) {
        animStateTransition(ANIM_LAND);
        stateTransition(NPC_STATE_DISABLED);
    } else if (m_brainState == BRAIN_KNOCKED_DOWN && !isAnimating()) {
        MathLine down(m_position.x, m_position.y, m_position.z, 0.0f, -1.0f, 0.0f);
        CollShape* below = m_map->getAttributeShapeAt(down, 0);
        if (below != NULL && below->isRamp()) {
            animStateTransition(ANIM_LAND);
            stateTransition(NPC_STATE_DISABLED);
            brainStateTransition(BRAIN_FALLEN);
        }
    }
    m_objectNode->setTranslation(m_position.x, m_position.y + m_modelOffsetY, m_position.z);
}

void GameObjectPolice::update(int dt) {
#ifdef MEDECOMP_PORT
    port::net::enemyAimBegin(this);
#endif
    m_distToPlayerSq = (int)getDistanceToSq(m_player);
    GameObjectNPC::update(dt);
    updateBrain(dt);
    updateState(dt);
    updatePosition(dt);
    updateAnimState(dt);
    updateFacing(dt);

    if (!m_combatStarted && fabsf(m_position.z) <= 0.5f && (float)m_distToPlayerSq <= 36.0f &&
        midp::Math::abs(m_position.y - m_player->m_position.y) <= 4.0f && m_fallenTime < 250 &&
        checkLOSWithPlayer()) {
        m_combatStarted = true;
        AppEngine::getCanvas()->getSceneGame()->startCombatTime();
    }

    MathVector decel = m_deceleration * (dt * 0.001f);
    MathVector prev(m_velocity);
    m_velocity.x -= m_velocity.x > 0 ? decel.x : -decel.x;
    if (prev.x * m_velocity.x <= 0) {
        m_velocity.x = 0;
        m_deceleration.x = 0;
    }
    m_velocity.y -= m_velocity.y > 0 ? decel.y : -decel.y;
    if (prev.y * m_velocity.y <= 0) {
        m_velocity.y = 0;
        m_deceleration.y = 0;
    }
    m_velocity.z -= m_velocity.z > 0 ? decel.z : -decel.z;
    if (prev.z * m_velocity.z <= 0) {
        m_velocity.z = 0;
        m_deceleration.z = 0;
    }
#ifdef MEDECOMP_PORT
    port::net::enemyAimEnd();
#endif
}

void GameObjectPolice::collidedWith(GameObject* other) {
    if (!isActive() || !other->isFlagSet(FLAG_PLAYER)) {
        return;
    }
    float distSq = getLateralDistanceToSq(m_player);
    bool attacking = m_player->isSliding() || m_player->isKicking() || m_player->isRolling();
    bool gettingUp = m_player->isGettingUp();
    bool down = m_player->isProne() || m_player->isOnRamp();

    if (attacking && distSq <= KNOCKDOWN_RANGE_SQ) {
        SpywareManager::getInstance()->trackGuardKilled(m_map->getUniqueGameObjectID(this));
        brainStateTransition(BRAIN_KNOCKED_DOWN);
        GameObjectPlayer* player = m_player;
        if (player->isKicking()) {
            player->kickSomething();
        }
        return;
    }
    GameObjectPlayer* player = m_player;
    bool disarming = player->isDisarming();
    if (disarming && distSq <= KNOCKDOWN_RANGE_SQ) {
        SpywareManager::getInstance()->trackGuardKilled(m_map->getUniqueGameObjectID(this));
        brainStateTransition(BRAIN_DISARMED);
        m_player->disarmCop(this);
        return;
    }
    if (attacking || m_brainState == BRAIN_MELEE || disarming) {
        return;
    }
    bool lying = player->isLying();
    if (distSq > MELEE_RANGE_SQ || lying || gettingUp || down) {
        return;
    }
    stateTransition(STATE_STAND);
    brainStateTransition(BRAIN_MELEE);
    m_player->meleeByCop(this);
    setFacingDir(m_player->m_position.x < m_position.x ? FACING_LEFT : FACING_RIGHT);
}

void GameObjectPolice::resetLevel() {
    m_combatStarted = false;
    m_fallenTime = 0;
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    m_modelOffsetY = 0;
    m_player = player;
    GameObject::resetLevel();
    stateTransition(NPC_STATE_START);
    animStateTransition(ANIM_IDLE);
    brainStateTransition(m_pathIndex != -1 ? BRAIN_PATROL : BRAIN_IDLE);
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
    m_objectNode->setOrientation(0.0f, 0.0f, 0.0f, 1.0f);
}

GameObjectPolice::~GameObjectPolice() {}

GameObjectPolice::GameObjectPolice(MEdgeMap* map, int type, int modelId, int blenderChannels, float x, float y,
                                   float z, int pathIndex)
    : GameObjectNPC(map, type, x, y, z),
      m_state(NPC_STATE_DISABLED),
      m_brainState(BRAIN_IDLE),
      m_animState(ANIM_IDLE),
      m_stateTime(0),
      m_fallenTime(0),
      m_combatStarted(false),
      m_deceleration(0.0f, 0.0f, 0.0f),
      m_testAnimIndex(-1),
      m_testAnimTimer(-1),
      m_aimTimer(-1),
      m_postureTimer(-1),
      m_postureCheckTimer(-1),
      m_shotTimer(-1),
      m_shotCheckTimer(-1),
      m_fireTimer(-1),
      m_pathIndex(pathIndex),
      m_path(NULL),
      m_pathNode(-1),
      m_modelOffsetY(0),
      m_player(NULL),
      m_distToPlayerSq(0),
      m_facePlayer(false),
      m_tuning() {
    setVisualAssets(modelId, blenderChannels);
    m_shape = new CollOrthoHexahedron(-0.8f, 0.0f, -0.3f, 0.8f, 1.8f, 0.3f);
    m_shape->setBounce(0);
    m_objectNodeCull = new GameObjectCulledGraphic(m_objectNode, getGlobalBounds());
    m_map->addCullObject(m_objectNodeCull);
    resetLevel();
    m_turnSpeed = TURN_SPEED;
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    if (!sound.isEventLoaded(SOUND_FALL_SCREAM)) {
        sound.loadEvent(SOUND_FALL_SCREAM);
    }
}
