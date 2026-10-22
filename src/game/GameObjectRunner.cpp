#include "game/GameObjectRunner.h"

#include "game/AnimationBlender.h"
#include "game/AnimationManager3D.h"
#include "game/AnimPlayer3D.h"
#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectRunnerData.h"
#include "game/M3GAssets.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"
#include "runtime/midp/Math.h"

const short GameObjectRunner::RESOURCE_ARRAY[] = {
     24,  79,  31,  27,  80,  32,  25,  77,  29,  28,  78,  30,
     23,  76,  28,  26,  81,  33,  51,  85,  57,  52,  84,  56,
     49,  83,  54,  48,  82,  53,  60,  87,  64,  59,  88,  65,
     61,  89,  67,   7,  12,   5,  10,   6,  11,  56,  61,  57,
     62,  58,  63,  11,  16,  12,  17,  54,  59,  55,  60,  50,
     55,   4,   9,   1,   6,   3,   8,   9,  14,  10,  15,  34,
     41,  35,  42,  13,  18,  18,  23,  15,  20,  29,  36,  30,
     37,  47,  35,  46,  34,  20,  25,  21,  26,  22,  27,  42,
     49,  45,  52,  62,  68,  63,  69,  64,  70,  65,  71,  66,
     72,  67,  75,  68,   1,  69,   2,  70,   3,  71,   4,  72,
    143,  73, 144,   0,   2,  36,  38,  43,  37,  39,  41,  46,
     40,  31,  32,  38,  33,  44,  43,   0,
};

const short GameObjectRunner::ANIM_FLAG_ARRAY[] = {4, 16};

GameObjectRunner::GameObjectRunner(MEdgeMap* map, int type, float x, float y, float z)
    : GameObject(map, type, x, y, z),
      m_blender(NULL),
      m_originNode(NULL),
      m_originAnimPlayer(NULL),
      m_originStartY(0),
      m_originYScale(0),
      m_originHeight(0),
      m_visualType(VISUAL_SOLO),
      m_visualIndex(-1),
      m_blendWeight(0),
      m_facingDir(FACING_RIGHT) {
    setVerticalAcceleration(GRAVITY);
}

GameObjectRunner::GameObjectRunner(MEdgeMap* map, int type)
    : GameObject(map, type),
      m_blender(NULL),
      m_originNode(NULL),
      m_originAnimPlayer(NULL),
      m_originStartY(0),
      m_originYScale(0),
      m_originHeight(0),
      m_visualType(VISUAL_SOLO),
      m_visualIndex(0),
      m_blendWeight(0),
      m_facingDir(FACING_RIGHT) {}

GameObjectRunner::~GameObjectRunner() {
    midp::DECREF(m_originAnimPlayer);
    m_originAnimPlayer = NULL;
    m_map->removeCullObject(m_objectNodeCull);
}

void GameObjectRunner::update(int dt) {
    GameObject::update(dt);
}

void GameObjectRunner::animationStopped() {}

void GameObjectRunner::setVisualAssets(int modelId, int blenderChannels) {
    AppEngine* engine = AppEngine::getCanvas();
    AnimationManager3D& animManager = engine->getAnimationManager3D();
    M3GAssets* assets = AppEngine::getM3GAssets();
    m3g::Group* world = engine->getSceneGame()->getM3GWorld();

    m3g::Group* group = new m3g::Group();
    m_objectNode = group;
    world->addChild(group);

    m_model = assets->loadModel(modelId, 4);
    group->addChild(m_model);

    m_animPlayer = new AnimPlayer3D(animManager);
    m_blender = new AnimationBlender(blenderChannels);
    m_blender->setNode(m_objectNode);

    setVisual(VISUAL_SOLO, 0);
    M3GAssets::cacheSkinTransforms(m_objectNode);
}

int GameObjectRunner::hashVisual(int type, int index, int facingDir) {
    return (index << 3) | (type << 1) | midp::Math::max(0, facingDir);
}

int GameObjectRunner::getVisual() {
    return hashVisual(m_visualType, m_visualIndex, m_facingDir);
}

void GameObjectRunner::setVisual(int visual) {
    setVisual((visual >> 1) & 3, visual >> 3);
    setFacingDir((FacingDir)(((visual & 1) << 1) - 1));
}

void GameObjectRunner::setVisual(int type, int index) {
    if (m_visualType == type && m_visualIndex == index) {
        return;
    }
    GameObjectRunnerData* data = AppEngine::getGameObjectRunnerData();
    if (type == VISUAL_BLEND3) {
        m_visualType = type;
        m_visualIndex = index;
        const GameObjectData_3ChannelBlendVisual& v = data->get3ChannelBlendAnim(index);
        m_animPlayer->startAnim(v.anim, 4);
        m_blender->setChannelWeight(v.channels[1], 1.0f, 3);
        m_blender->setChannelWeight(v.channels[0], 0.0f, 1);
        m_blender->setChannelWeight(v.channels[2], 0.0f, 1);
    } else if (type == VISUAL_ORIGIN) {
        const GameObjectData_OriginAnimVisual& v = data->getOriginAnim(index);
        if (m_originAnimPlayer != NULL) {
            if (v.height == 0.0f) {
                MathVector offset;
                calculateOffsetAnimationOverallOffset(index, offset);
                setOffsetVisual(index, offset.y);
            } else {
                setOffsetVisual(index, v.height);
            }
        } else {
            m_animPlayer->startAnim(v.anim, 16);
            m_blender->setChannelWeight(v.channel, 1.0f, v.blendFlags | 3);
        }
    } else if (type == VISUAL_SOLO) {
        m_visualType = type;
        m_visualIndex = index;
        const GameObjectData_SoloVisual& v = data->getSoloAnim(index);
        m_animPlayer->startAnim(v.anim, v.animFlags);
        m_blender->setChannelWeight(v.channel, 1.0f, v.blendFlags | 3);
    }
    m_blendWeight = 0;
}

void GameObjectRunner::setOffsetVisual(int index, float height) {
    if (m_visualType == VISUAL_ORIGIN && m_visualIndex == index) {
        return;
    }
    m_visualType = VISUAL_ORIGIN;
    m_visualIndex = index;
    const GameObjectData_OriginAnimVisual& v = AppEngine::getGameObjectRunnerData()->getOriginAnim(index);
    m_animPlayer->startAnim(v.anim, 16);
    m_blender->setChannelWeight(v.channel, 1.0f, v.blendFlags | 3);

    if (m_originAnimPlayer != NULL) {
        MathVector start;
        MathVector overall;
        calculateOffsetAnimationStartOffset(index, start);
        calculateOffsetAnimationOverallOffset(index, overall);
        m_originAnimPlayer->startAnim(v.originAnim, 16);
        m_originBasePos.set(m_position.x - start.x, m_position.y, m_position.z);
        m_preOriginAnimPos = m_position;
        m_originStartY = start.y;
        if (overall.y == height) {
            m_originYScale = 1.0f;
        } else {
            m_originYScale = height / overall.y;
        }
        m_originHeight = height;
    }
}

int GameObjectRunner::getVisualDuration(int type, int index) {
    AnimationManager3D& animManager = AppEngine::getCanvas()->getAnimationManager3D();
    int anim;
    switch (type) {
    case VISUAL_SOLO:
        anim = AppEngine::getGameObjectRunnerData()->getSoloAnim(index).anim;
        break;
    case VISUAL_ORIGIN:
        anim = AppEngine::getGameObjectRunnerData()->getOriginAnim(index).anim;
        break;
    case VISUAL_BLEND3:
        anim = AppEngine::getGameObjectRunnerData()->get3ChannelBlendAnim(index).anim;
        break;
    default:
        return -1;
    }
    return animManager.getAnimationDuration(anim);
}

int GameObjectRunner::getVisualType() {
    return m_visualType;
}

int GameObjectRunner::getVisualIndex() {
    return m_visualIndex;
}

bool GameObjectRunner::isInOriginAnimation() {
    return m_visualType == VISUAL_ORIGIN;
}

GameObjectRunner::FacingDir GameObjectRunner::getFacingDir() const {
    return m_facingDir;
}

void GameObjectRunner::setFacingDir(FacingDir dir) {
    m_facingDir = dir;
    if (m_model != NULL) {
        m_model->setOrientation(dir * 90.0f, 0.0f, 1.0f, 0.0f);
    }
}

void GameObjectRunner::reverseFacingDir() {
    setFacingDir((FacingDir)-m_facingDir);
}

float GameObjectRunner::get3WayAnimationBlendWeights() {
    return m_blendWeight;
}

void GameObjectRunner::set3WayAnimationBlendWeights(float weight) {
    m_blendWeight = weight;
    const GameObjectData_3ChannelBlendVisual& v =
        AppEngine::getGameObjectRunnerData()->get3ChannelBlendAnim(m_visualIndex);
    m_blender->setChannelWeight(v.channels[0], midp::Math::max(0.0f, midp::Math::min(1.0f, -weight)), 0);
    m_blender->setChannelWeight(v.channels[1], midp::Math::max(0.0f, 1.0f - midp::Math::abs(weight)), 0);
    m_blender->setChannelWeight(v.channels[2], midp::Math::max(0.0f, midp::Math::min(1.0f, weight)), 0);
}

float GameObjectRunner::calcAnimProgress() const {
    AnimPlayer3D* player = const_cast<GameObjectRunner*>(this)->m_animPlayer;
    return (float)player->getAnimTime() / (float)player->getAnimDuration();
}

void GameObjectRunner::updateAnimation(int dt) {
    bool culled = m_objectNodeCull->isFullyCulled();
    m_objectNode->setRenderingEnable(true);
    m_animPlayer->updateAnim(dt);
    if (m_objectNode != NULL) {
        m_blender->update(dt, !culled);
    }
    if (m_originAnimPlayer != NULL && m_visualType == VISUAL_ORIGIN) {
        m_originAnimPlayer->updateAnim(dt);
    }
    if (isAnimating()) {
        return;
    }

    if (m_visualType == VISUAL_ORIGIN && m_originAnimPlayer != NULL) {
        const GameObjectData_OriginAnimVisual& v =
            AppEngine::getGameObjectRunnerData()->getOriginAnim(m_visualIndex);
        m_originAnimPlayer->setAnimTime(m_originAnimPlayer->getAnimDuration());
        float t[3];
        m_originNode->getTranslation(t);
        MathVector pos = getForwardDirection();
        pos *= getFacingDir() * t[2];
        pos.y = m_originHeight;
        pos += m_originBasePos;
        bool blocked = false;
        if (v.collide) {
            m_shape->setPosition(pos);
            blocked = m_map->intersects(m_shape, 0);
        }
        if (!blocked) {
            m_position = pos;
        }
    }
    animationStopped();
}

void GameObjectRunner::updateMovementOriginAnimation(int facingDir, int dt) {
    if (m_originAnimPlayer == NULL || m_visualType != VISUAL_ORIGIN) {
        return;
    }
    const GameObjectData_OriginAnimVisual& v =
        AppEngine::getGameObjectRunnerData()->getOriginAnim(m_visualIndex);
    float t[3];
    m_originNode->getTranslation(t);
    MathVector pos = getForwardDirection();
    pos *= facingDir * t[2];
    pos.y = (t[1] - m_originStartY) * m_originYScale;
    pos += m_originBasePos;
    bool blocked = false;
    if (v.collide) {
        m_shape->setPosition(pos);
        blocked = m_map->intersects(m_shape, 0);
    }
    if (!blocked) {
        m_position = pos;
    }

    if (v.useGravity) {
        float seconds = dt * 0.001f;
        setForwardVelocity(0.0f);
        m_velocity.y += seconds * GRAVITY;
        MathVector moved(m_position);
        if (m_map->moveObject(moved, m_velocity, seconds, m_shape) & MEdgeMap::HIT_FLOOR) {
            m_velocity.y = 0;
        }
        m_originBasePos.y += moved.y - m_position.y;
        m_position.y = moved.y;
    } else {
        setNoVelocity();
    }
}

void GameObjectRunner::calculateOffsetAnimationStartOffset(int index, MathVector& out) {
    const GameObjectData_OriginAnimVisual& v = AppEngine::getGameObjectRunnerData()->getOriginAnim(index);
    m_originAnimPlayer->startAnim(v.anim, 16);
    float t[3];
    m_originNode->getTranslation(t);
    out.set(m_facingDir * t[2], t[1], 0.0f);
}

void GameObjectRunner::calculateOffsetAnimationOverallOffset(int index, MathVector& out) {
    const GameObjectData_OriginAnimVisual& v = AppEngine::getGameObjectRunnerData()->getOriginAnim(index);
    m_originAnimPlayer->startAnim(v.anim, 16);
    float start[3];
    m_originNode->getTranslation(start);
    m_originAnimPlayer->setAnimTime(m_originAnimPlayer->getAnimDuration());
    float end[3];
    m_originNode->getTranslation(end);
    out.set(m_facingDir * (end[2] - start[2]), end[1] - start[1], 0.0f);
}

float GameObjectRunner::getOriginAnimationYOffset(int index) {
    float height = AppEngine::getGameObjectRunnerData()->getOriginAnim(index).height;
    if (height == 0.0f) {
        MathVector offset;
        calculateOffsetAnimationOverallOffset(index, offset);
        height = offset.y;
    }
    return height;
}

MathVector GameObjectRunner::getPreOriginAnimPosNoOffset() {
    return m_preOriginAnimPos;
}
