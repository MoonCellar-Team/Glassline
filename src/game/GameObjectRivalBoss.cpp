#include "game/GameObjectRivalBoss.h"

#include "game/AppEngine.h"
#include "game/GameObjectPlayer.h"
#include "game/LevelData.h"
#include "game/SceneGame.h"

#include <math.h>

static const int MODEL_BOSS = 16;

static const float SEEN_RANGESQ = 81.0f;
static const float ATTACK_RANGESQ = 36.0f;

bool GameObjectRivalBoss::shouldUpdate() {
    if (!m_playerSeen) {
        m_playerSeen = (float)m_distToPlayerSq < SEEN_RANGESQ;
    }
    if (m_notFinalFight) {
        return GameObjectRival::shouldUpdate();
    }
    return m_playerSeen;
}

void GameObjectRivalBoss::update(int dt) {
    GameObjectRival::update(dt);
    if (m_notFinalFight) {
        return;
    }
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    if ((float)m_distToPlayerSq < ATTACK_RANGESQ && fabsf(player->m_position.y - m_position.y) < 0.5f &&
        m_brainState == BRAIN_FOLLOW_PATH) {
        bool ahead = false;
        MathVector fromPlayer = m_position - player->m_position;
        if (fromPlayer.x * m_velocity.x < 0) {
            ahead = true;
            if (!m_slideDecided && AppEngine::getCanvas()->randPercent() < 50.0f) {
                brainStateTransition(BRAIN_SLIDE);
                m_slideTimer = 1000;
            }
        }
        m_slideDecided = ahead;
    } else if (m_slideTimer > 0) {
        m_slideTimer -= dt;
        if (m_slideTimer <= 0) {
            brainStateTransition(BRAIN_FOLLOW_PATH);
            m_slideDecided = false;
        }
    }
    if (m_combatStarted && (float)m_distToPlayerSq > ATTACK_RANGESQ) {
        m_combatStarted = false;
    }
}

void GameObjectRivalBoss::resetLevel() {
    GameObjectRival::resetLevel();
    m_slideDecided = false;
    m_slideTimer = 0;
    m_playerSeen = false;
}

GameObjectRivalBoss::GameObjectRivalBoss(MEdgeMap* map, float x, float y, float z, int pathIndex)
    : GameObjectRival(map, TYPE_RIVAL_BOSS, MODEL_BOSS, 0, x, y, z, pathIndex),
      m_notFinalFight(false),
      m_slideDecided(false),
      m_slideTimer(0),
      m_playerSeen(false) {
    m_notFinalFight = !AppEngine::getLevelData()->isCurrentLevelLastStory();
}

GameObjectRivalBoss::~GameObjectRivalBoss() {}
