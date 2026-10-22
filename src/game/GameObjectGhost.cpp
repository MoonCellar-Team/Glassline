#include "game/GameObjectGhost.h"

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectPlayer.h"
#include "game/GhostAnimationPlayback.h"
#include "game/GhostKeyframe.h"
#include "game/M3GAssets.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"
#include "runtime/midp/Math.h"

static const int TYPE_GHOST = 2;

static const int MODEL_GHOST = 13;

static const int NODE_REMOVED_1 = 103;
static const int NODE_REMOVED_2 = 104;

GameObjectGhost::GameObjectGhost(MEdgeMap* map, GhostAnimationPlayback* playback)
    : GameObjectRunner(map, TYPE_GHOST),
      m_playback(playback),
      m_keyframeIndex(0),
      m_keyframeTime(0),
      m_separated(false),
      m_fadeInTime(0),
      m_time(0),
      m_duration(0) {
    setVisualAssets(MODEL_GHOST, 0);
    M3GAssets::orphanNode(static_cast<m3g::Node*>(m_objectNode->find(NODE_REMOVED_1)));
    M3GAssets::orphanNode(static_cast<m3g::Node*>(m_objectNode->find(NODE_REMOVED_2)));
    M3GAssets::applyAlphaFactor(m_objectNode, 0.0f);

    m_shape = new CollOctahedron(-0.5f, 0.0f, -0.5f, 0.5f, 1.8f, 0.5f);
    m_objectNodeCull = new GameObjectCulledGraphic(m_objectNode, getGlobalBounds());
    m_map->addCullObject(m_objectNodeCull);

    int count = m_playback->getKeyframeNum();
    for (int i = 0; i != count; i++) {
        m_duration += m_playback->getKeyframe(i)->m_deltaTime;
    }
}

void GameObjectGhost::checkpointActivated() {}

void GameObjectGhost::resetCheckpoint() {}

void GameObjectGhost::update(int dt) {
    SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
    GameObjectPlayer* player = scene->getSinglePlayerObject();
    if (scene->getState() == SceneGame::STATE_RACE_START) {
        return;
    }
    GameObjectRunner::update(dt);
    m_keyframeTime += dt;
    m_time += dt;

    int last = m_playback->getKeyframeNum() - 1;
    bool keyframeChanged = false;
    GhostKeyframe* keyframe = m_playback->getKeyframe(m_keyframeIndex);
    while (m_keyframeIndex != last && keyframe->m_deltaTime <= m_keyframeTime) {
        m_keyframeTime -= keyframe->m_deltaTime;
        keyframeChanged = true;
        m_keyframeIndex++;
        keyframe = m_playback->getKeyframe(m_keyframeIndex);
    }
    if (keyframeChanged) {
        setVisual(keyframe->m_visual);
        if (getVisualType() == VISUAL_BLEND3) {
            set3WayAnimationBlendWeights(keyframe->m_blendWeight);
        }
    }

    int index = m_keyframeIndex;
    if (index != last && m_keyframeTime != 0) {
        GhostKeyframe* next = m_playback->getKeyframe(index + 1);
        m_position.setAsLinearInterpolation(keyframe->m_position, next->m_position,
                                            (float)m_keyframeTime / (float)keyframe->m_deltaTime);
    } else {
        m_position = keyframe->m_position;
    }
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);

    if (!m_separated) {
        if (!GameCommon::compareFloats(m_position.x, player->m_position.x) ||
            !GameCommon::compareFloats(m_position.y, player->m_position.y) ||
            !GameCommon::compareFloats(m_position.z, player->m_position.z)) {
            m_separated = true;
        }
    } else if (m_fadeInTime < FADE_TIME) {
        m_fadeInTime = midp::Math::min(m_fadeInTime + dt, FADE_TIME);
        M3GAssets::applyAlphaFactor(m_objectNode, (float)m_fadeInTime * 0.001f);
    }

    if (m_duration - m_time <= FADE_TIME) {
        int timeLeft = midp::Math::max(0, m_duration - m_time);
        M3GAssets::applyAlphaFactor(m_objectNode, (float)timeLeft * 0.001f);
    }
}

void GameObjectGhost::resetLevel() {
    m_separated = false;
    m_fadeInTime = 0;
    m_time = 0;
    m_keyframeIndex = 0;
    m_keyframeTime = 0;
    M3GAssets::applyAlphaFactor(m_objectNode, 0.0f);
    resetCheckpoint();

    GhostKeyframe* first = m_playback->getKeyframe(0);
    setVisual(first->m_visual);
    m_position.set(first->m_position);
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
}

GameObjectGhost::~GameObjectGhost() {}
