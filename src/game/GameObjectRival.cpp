#include "game/GameObjectRival.h"

#include <math.h>

#include "game/AchievementData.h"
#include "game/AnimPlayer3D.h"
#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectPlayer.h"
#include "game/MathLine.h"
#include "game/MEdgeMap.h"
#include "game/Path.h"
#include "game/PathPoint.h"
#include "game/SceneGame.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "m3g/Node.h"

static const int FLAG_PLAYER = 1;

static const int SCENE_STATE_2 = 2;

static const int BOSS_KNOCKDOWNS = 2;

static const int SOUND_POOL_HIT = 5;
static const int SOUND_GROUP_HIT = 1;

static float TURN_SPEED = midp::Math::PI * 4.0f;

static int iabs(int v) {
    return v < 0 ? -v : v;
}

const int GameObjectRival::UPDATE_RANGESQ = 400;

const int GameObjectRival::testAnims[NUM_TEST_ANIMS] = {12, 5, 6, 7, 13, 14, 15, 16};

const int GameObjectRival::testChannels[NUM_TEST_ANIMS] = {7, 0, 1, 2, 8, 9, 10, 11};

bool GameObjectRival::onGround() {
    return (unsigned)(m_state - STATE_JUMP) > 1;
}

bool GameObjectRival::shouldUpdate() {
    if (m_state == NPC_STATE_DISABLED) {
        return false;
    }
    if (AppEngine::getCanvas()->getSceneGame()->getState() == SCENE_STATE_2) {
        return false;
    }
    return m_distToPlayerSq <= UPDATE_RANGESQ;
}

void GameObjectRival::updateTestAnims(int dt) {
    m_testAnimTimer -= dt;
    if (m_testAnimTimer > 0) {
        return;
    }
    m_testAnimIndex = (m_testAnimIndex + 1) % NUM_TEST_ANIMS;
    startBlendedAnim(testAnims[m_testAnimIndex], testChannels[m_testAnimIndex], 4);
    m_testAnimTimer = 5000;
}

void GameObjectRival::updateState(int dt) {
    if (m_state == NPC_STATE_TEST_ANIMS) {
        updateTestAnims(dt);
    } else if (m_state == NPC_STATE_START) {
        stateTransition(NPC_STATE_ACTIVE);
    }
}

void GameObjectRival::animStateTransition(AnimState state) {
    if (m_animState == state) {
        return;
    }
    switch (state) {
    case ANIM_RUN:
        startBlendedAnim(6, 1, 4);
        break;
    case ANIM_JUMP:
        startBlendedAnim(14, 9, 16);
        break;
    case ANIM_SLIDE:
        startBlendedAnim(26, 21, 16);
        break;
    case ANIM_WALL_RUN:
        if (m_velocity.x > 0) {
            startBlendedAnim(20, 15, 4);
        } else {
            startBlendedAnim(23, 18, 4);
        }
        break;
    case ANIM_CLIMB:
        startBlendedAnim(18, 13, 4);
        break;
    case ANIM_CLAMBER:
        startBlendedAnim(31, 24, 16);
        break;
    case ANIM_RAMP_DOWN:
        startBlendedAnim(38, 31, 4);
        break;
    case ANIM_RAMP_UP:
        startBlendedAnim(8, 3, 4);
        break;
    case ANIM_KNOCKED_DOWN:
        startBlendedAnim(64, 60, 16);
        break;
    case ANIM_GET_UP:
        startBlendedAnim(69, 63, 16);
        break;
    default:
        break;
    }
    m_animState = state;
}

void GameObjectRival::brainStateTransition(BrainState state) {
    if (m_brainState == state) {
        return;
    }
    switch (state) {
    case BRAIN_FOLLOW_PATH:
        stateTransition(STATE_RUN);
        animStateTransition(ANIM_RUN);
        break;
    case BRAIN_JUMP:
    case BRAIN_JUMP_TO:
        animStateTransition(ANIM_JUMP);
        stateTransition(STATE_JUMP);
        break;
    case BRAIN_SLIDE:
        animStateTransition(ANIM_SLIDE);
        stateTransition(STATE_SLIDE);
        break;
    case BRAIN_WALL_RUN:
        animStateTransition(ANIM_WALL_RUN);
        break;
    case BRAIN_CLIMB:
        animStateTransition(ANIM_CLIMB);
        break;
    case BRAIN_CLAMBER:
        animStateTransition(ANIM_CLAMBER);
        break;
    case BRAIN_KNOCKED_DOWN:
        setNoVelocity();
        m_savedBrainState = m_brainState;
        animStateTransition(ANIM_KNOCKED_DOWN);
        break;
    default:
        break;
    }
    m_brainState = state;
}

void GameObjectRival::processWallRunNode(MathVector& toNode) {
    brainStateTransition(BRAIN_WALL_RUN);
}

void GameObjectRival::processSlideNode(MathVector& toNode) {
    brainStateTransition(BRAIN_SLIDE);
}

void GameObjectRival::processClimbNode(MathVector& toNode) {
    brainStateTransition(BRAIN_CLIMB);
}

void GameObjectRival::processJumpToNode(MathVector& toNode) {
    brainStateTransition(BRAIN_JUMP_TO);
}

void GameObjectRival::processJumpNode(MathVector& toNode) {
    float flightTime = (float)iabs((int)toNode.x) / (float)iabs((int)m_velocity.x);
    setUpVelocity(-(GRAVITY * flightTime * 0.5f));
    brainStateTransition(BRAIN_JUMP);
}

void GameObjectRival::brainUpdateClamber(int dt) {
    m_velocity.y = 0;
    if (!m_animPlayer->isAnimating()) {
        brainStateTransition(BRAIN_FOLLOW_PATH);
    }
}

MathVector GameObjectRival::getVectorToNextPathNode() {
    MathVector v(*m_path->getPoint(m_pathNode));
    v -= m_position;
    return v;
}

bool GameObjectRival::onReachedNode(MathVector& toNode) {
    const PathPoint* node = m_path->getPoint(m_pathNode);
    int action = node->m_action;
    m_speed = node->m_speed * 0.001f;
    m_pathNode = (m_pathNode + 1) % m_path->getPointCount();
    toNode = getVectorToNextPathNode();

    if ((unsigned)(m_brainState - BRAIN_JUMP_TO) <= BRAIN_CLIMB - BRAIN_JUMP_TO) {
        brainStateTransition(BRAIN_FOLLOW_PATH);
    }
    switch (action) {
    case NODE_JUMP:
        processJumpNode(toNode);
        return false;
    case NODE_JUMP_TO:
        processJumpToNode(toNode);
        return false;
    case NODE_CLIMB:
        processClimbNode(toNode);
        return true;
    case NODE_SLIDE:
        processSlideNode(toNode);
        return true;
    case NODE_WALL_RUN:
        processWallRunNode(toNode);
        return false;
    default:
        return true;
    }
}

void GameObjectRival::brainUpdateClimb(int dt) {
    MathVector toNode = getVectorToNextPathNode();
    if (toNode.y < 0) {
        setUpVelocity(0);
        onReachedNode(toNode);
    } else {
        setUpVelocity(m_speed);
        toNode.setLength(m_speed);
        m_velocity.x = toNode.x;
    }
}

void GameObjectRival::brainUpdateOnRails(int dt) {
    MathVector toNode = getVectorToNextPathNode();
    if (toNode.getLength() > 0.7f) {
        toNode.setLength(m_speed);
        m_velocity = toNode;
        m_targetFacing = facingFromDirection(m_velocity.x, m_velocity.z);
    } else {
        onReachedNode(toNode);
    }
}

bool GameObjectRival::hasPastPathNode() {
    MathVector toNode = getVectorToNextPathNode();
    toNode.y = 0;
    if (toNode.getLength() <= 0.7f) {
        return true;
    }
    if (m_pathNode < 1) {
        return false;
    }
    MathVector prev(*m_path->getPoint(m_pathNode - 1));
    MathVector next(*m_path->getPoint(m_pathNode));
    MathVector segment = next - prev;
    MathVector fromPrev = m_position - prev;
    return segment.getLengthSq() < fromPrev.getLengthSq();
}

void GameObjectRival::brainUpdateFollowPath(int dt) {
    if (!onGround()) {
        return;
    }
    MathVector toNode = getVectorToNextPathNode();
    if (hasPastPathNode() && !onReachedNode(toNode)) {
        return;
    }
    MathVector dir(toNode);
    dir.y = 0;
    if (dir.getLengthSq() > 0) {
        dir.setLength(m_speed);
    }
    m_velocity.x = dir.x;
    m_velocity.z = dir.z;
    m_targetFacing = facingFromDirection(m_velocity.x, m_velocity.z);
}

void GameObjectRival::updatePosition(int dt) {
    float seconds = dt * 0.001f;

    if ((unsigned)(m_state - STATE_RAMP_UP) <= 1) {
        MathVector dir = m_rampEnd - m_rampStart;
        dir.z = 0;
        dir.setLength(m_speed);
        m_position += dir * seconds;
        MathVector total = m_rampEnd - m_rampStart;
        total.z = 0;
        MathVector done = m_position - m_rampStart;
        done.z = 0;
        if (total.getLengthSq() < done.getLengthSq()) {
            stateTransition(STATE_RUN);
            animStateTransition(ANIM_RUN);
        }
        m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
        return;
    }

    MathVector normal;
    CollShape* hit = NULL;
    MathVector velocity = m_velocity * 1.0f;
    int flags = m_map->moveObjectPhysics(m_position, velocity, seconds, m_shape, &hit, normal);
    m_velocity = velocity;
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);

    if (flags & MEdgeMap::HIT_FLOOR) {
        if (m_state == STATE_FALL) {
            stateTransition(STATE_RUN);
            if (m_brainState == BRAIN_JUMP) {
                brainStateTransition(BRAIN_FOLLOW_PATH);
            }
        }
        if (hit->isRamp() && ((hit->getShapeType() == 2 && m_velocity.x > 0) ||
                              (hit->getShapeType() == 1 && m_velocity.x < 0))) {
            m_rampShape = hit;
            stateTransition(STATE_RAMP_UP);
            animStateTransition(ANIM_RAMP_UP);
            return;
        }
    } else if (m_velocity.y < 0) {
        MathLine down(m_position.x, m_position.y, m_position.z, 0.0f, -1.0f, 0.0f);
        CollShape* below = m_map->getAttributeShapeAt(down, 0);
        if (below != NULL && below->isRamp() && fabsf(m_position.y - below->getBounds().m_max.y) < 0.5f) {
            if ((below->getShapeType() == 2 && m_velocity.x < 0) ||
                (below->getShapeType() == 1 && m_velocity.x > 0)) {
                m_rampShape = below;
                stateTransition(STATE_RAMP_DOWN);
                animStateTransition(ANIM_RAMP_DOWN);
                return;
            }
        } else {
            stateTransition(STATE_FALL);
        }
    }

    float pi = midp::Math::PI;
    if (((flags & MEdgeMap::HIT_RIGHT) && m_facing > pi) || ((flags & MEdgeMap::HIT_LEFT) && m_facing < pi)) {
        float offSide = fabsf(m_facing + midp::Math::PI * -0.5f);
        MathVector ahead = getForwardDirection();
        MathVector up(0.0f, 1.0f, 0.0f);
        ahead *= offSide >= 0.01f ? 0.51f : -0.51f;
        ahead += m_position;
        MathLine line(ahead, up);
        const MathOrthoBox& box = m_shape->getLocalBounds();
        float tMin = box.m_min.y;
        float tMax = box.m_max.y;
        if (m_map->intersects(line, 0, tMin, tMax) && tMax < box.m_max.y) {
            m_position.y = m_position.y + tMax + 0.2f;
            setNoVelocity();
            brainStateTransition(BRAIN_CLAMBER);
        }
    }
}

void GameObjectRival::stateTransition(NPCState state) {
    if (m_state == state) {
        return;
    }
    if ((unsigned)(m_state - STATE_RAMP_UP) <= 1) {
        m_objectNode->setOrientation(0.0f, 0.0f, 0.0f, 1.0f);
    }

    switch (state) {
    case STATE_SLIDE:
        m_shape = m_slideShape;
        break;
    case STATE_RUN:
        m_shape = m_standShape;
        break;
    case STATE_RAMP_UP: {
        m_rampStart = m_position;
        CollShape* ramp = m_rampShape.operator->();
        const MathOrthoBox& bounds = ramp->getBounds();
        if (ramp->getShapeType() == 2) {
            m_rampEnd = bounds.m_max;
        } else {
            m_rampEnd = MathVector(bounds.m_min.x, bounds.m_max.y, 0.0f);
        }
        m_rampShape = midp::ReferenceCountedPointer<CollShape>(NULL);
        break;
    }
    case STATE_RAMP_DOWN: {
        m_rampStart = m_position;
        CollShape* ramp = m_rampShape.operator->();
        const MathOrthoBox& bounds = ramp->getBounds();
        if (ramp->getShapeType() == 2) {
            m_rampEnd = bounds.m_min;
            m_objectNode->setOrientation(30.0f, 0.0f, 0.0f, 1.0f);
        } else {
            m_rampEnd = MathVector(bounds.m_max.x, bounds.m_min.y, 0.0f);
            m_objectNode->setOrientation(-30.0f, 0.0f, 0.0f, 1.0f);
        }
        m_rampEnd.y += 0.3f;
        m_rampShape = midp::ReferenceCountedPointer<CollShape>(NULL);
        break;
    }
    default:
        break;
    }
    m_state = state;
    m_stateTime = 0;
}

void GameObjectRival::updateBrain(int dt) {
    switch (m_brainState) {
    case BRAIN_FOLLOW_PATH:
    case BRAIN_SLIDE:
        brainUpdateFollowPath(dt);
        break;
    case BRAIN_JUMP_TO:
    case BRAIN_WALL_RUN:
        brainUpdateOnRails(dt);
        break;
    case BRAIN_CLIMB:
        brainUpdateClimb(dt);
        break;
    case BRAIN_CLAMBER:
        brainUpdateClamber(dt);
        break;
    case BRAIN_KNOCKED_DOWN:
        if (m_animPlayer->isAnimating() || getType() != TYPE_RIVAL_BOSS) {
            break;
        }
        if (m_timesKnockedDown > BOSS_KNOCKDOWNS) {
            SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
            if (scene->getState() == SceneGame::STATE_GAME) {
                scene->completeLevel();
            }
        } else {
            brainStateTransition(BRAIN_GET_UP);
        }
        break;
    case BRAIN_GET_UP:
        if (!m_animPlayer->isAnimating()) {
            brainStateTransition(m_savedBrainState);
        }
        break;
    default:
        break;
    }
}

void GameObjectRival::update(int dt) {
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    m_distToPlayerSq = (int)getDistanceToSq(player);
    if (!shouldUpdate()) {
        return;
    }
    GameObjectNPC::update(dt);
    updateBrain(dt);
    updateState(dt);
    updatePosition(dt);
    updateFacing(dt);

    bool near = false;
    bool playerAhead = (player->m_position.x < m_position.x) == (m_facing < midp::Math::PI);
    if (playerAhead) {
        near = (float)m_distToPlayerSq <= 144.0f;
    } else if (getType() != TYPE_RIVAL_BOSS) {
        near = (float)m_distToPlayerSq <= 2.25f;
    }

    if (!m_combatStarted && fabsf(m_position.z) <= 0.5f && near &&
        midp::Math::abs(m_position.y - player->m_position.y) <= 4.0f) {
        if (!checkLOSWithPlayer()) {
            return;
        }
        m_combatStarted = true;
        AppEngine::getCanvas()->getSceneGame()->startCombatTime();
    }
}

void GameObjectRival::collidedWith(GameObject* other) {
    if (m_state == NPC_STATE_DISABLED) {
        return;
    }
    if ((unsigned)(m_brainState - BRAIN_KNOCKED_DOWN) < 2) {
        return;
    }
    if (!other->isFlagSet(FLAG_PLAYER)) {
        return;
    }
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    if (player->isSliding() || player->isKicking() || player->isRolling()) {
        if (m_brainState == BRAIN_FOLLOW_PATH && m_state != STATE_JUMP && m_state != STATE_FALL &&
            m_state != STATE_RAMP_DOWN) {
            brainStateTransition(BRAIN_KNOCKED_DOWN);
            m_timesKnockedDown++;
            AppEngine::getAchievementData()->registerRivalKill(
                AppEngine::getCanvas()->getSceneGame()->getRaceTime());
            AppEngine* app = AppEngine::getCanvas();
            SoundManager& sound = app->getSoundManager();
            sound.playEvent(app->getSceneGame()->getSoundPoolManager().getRandomSoundEventID(
                                SOUND_POOL_HIT, SOUND_GROUP_HIT, -1),
                            1.0f);
            return;
        }
    }
    if (m_brainState != BRAIN_SLIDE) {
        return;
    }
    if (player->isMeleed()) {
        return;
    }
    player->meleeByCop(this);
    AppEngine* app = AppEngine::getCanvas();
    SoundManager& sound = app->getSoundManager();
    sound.playEvent(app->getSceneGame()->getSoundPoolManager().getRandomSoundEventID(SOUND_POOL_HIT, SOUND_GROUP_HIT, -1),
                    1.0f);
}

void GameObjectRival::resetLevel() {
    GameObject::resetLevel();
    m_stateTime = 0;
    m_pathNode = 0;
    m_combatStarted = false;
    m_savedBrainState = BRAIN_NONE;
    m_timesKnockedDown = 0;
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
    brainStateTransition(BRAIN_FOLLOW_PATH);
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
}

GameObjectRival::~GameObjectRival() {}

GameObjectRival::GameObjectRival(MEdgeMap* map, int type, int modelId, int blenderChannels, float x, float y,
                                 float z, int pathIndex)
    : GameObjectNPC(map, type, x, y, z),
      m_state(NPC_STATE_DISABLED),
      m_brainState(BRAIN_NONE),
      m_savedBrainState(BRAIN_NONE),
      m_animState(ANIM_NONE),
      m_stateTime(0),
      m_speed(9.5f),
      m_combatStarted(false),
      m_testAnimIndex(-1),
      m_testAnimTimer(-1),
      m_path(m_map->getPath(pathIndex)),
      m_pathNode(0),
      m_rampShape(NULL),
      m_timesKnockedDown(0),
      m_distToPlayerSq(0) {
    setVisualAssets(modelId, blenderChannels);
    m_standShape = new CollOctahedron(-0.5f, 0.0f, -0.5f, 0.5f, 1.8f, 0.5f);
    m_standShape->setBounce(0);
    m_slideShape = new CollOctahedron(-0.5f, 0.0f, -0.5f, 0.5f, 0.5f, 0.5f);
    m_slideShape->setBounce(0);
    m_shape = m_standShape;
    m_objectNodeCull = new GameObjectCulledGraphic(m_objectNode, getGlobalBounds());
    m_map->addCullObject(m_objectNodeCull);
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
    brainStateTransition(BRAIN_FOLLOW_PATH);
    m_turnSpeed = TURN_SPEED;
}
