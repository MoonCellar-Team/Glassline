#include "game/GameObjectCheckpoint.h"

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectPlayer.h"
#include "game/GameScreen.h"
#include "game/MEdgeMap.h"
#include "game/ModelSet.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"
#include "runtime/midp/Math.h"

static const int FLAG_PLAYER = 1;

GameObjectRunner::FacingDir GameObjectCheckpoint::getPlayerFacingDir() {
    return m_playerFacingDir;
}

GameObjectCheckpoint::GameObjectCheckpoint(MEdgeMap* map, const ModelSet* modelSet, float minX, float minY,
                                           float maxX, float maxY, m3g::Node* graphic, bool facingRight)
    : GameObject(map, TYPE, 0.0f, 0.0f, 0.0f),
      m_dishUserId(modelSet->getUserId(1)),
      m_dish(NULL),
      m_offAngle(0),
      m_onAngle(0),
      m_dishState(DISH_OFF),
      m_dishTimer(0),
      m_playerFacingDir(GameObjectRunner::FACING_LEFT) {
    m_shape = new CollOrthoHexahedron(minX, minY, -1.0f, maxX, maxY, 1.0f);
    if (facingRight) {
        m_playerFacingDir = GameObjectRunner::FACING_RIGHT;
    }
    if (m_dishUserId == WIDE_DISH_USER_ID) {
        if (facingRight) {
            m_offAngle = 0.0f;
            m_onAngle = 136.0f;
        } else {
            m_offAngle = 136.0f;
            m_onAngle = 0.0f;
        }
    } else {
        if (facingRight) {
            m_offAngle = -45.0f;
            m_onAngle = 45.0f;
        } else {
            m_offAngle = 45.0f;
            m_onAngle = -45.0f;
        }
    }
    if (m_dishUserId != -1) {
        graphic = static_cast<m3g::Node*>(graphic->find(m_dishUserId));
    }
    m_dish = graphic;
    setActiveAngleFactor(0.0f);
}

void GameObjectCheckpoint::setActiveAngleFactor(float factor) {
    float eased;
    if (factor >= 0.5f) {
        float t = 1.0f - factor;
        t = t + t;
        eased = t * t * t * -0.5 + 1.0;
    } else {
        float t = factor + factor;
        eased = t * t * t * 0.5f;
    }
    float angle = m_offAngle * (1.0f - eased) + m_onAngle * eased;
    ModelSet* modelSet = m_map->getModelSet();
    m_dish->setOrientation(angle, 0.0f, -modelSet->getRotateY(), -modelSet->getRotateZ());
}

void GameObjectCheckpoint::resetLevel() {
    m_dishState = DISH_OFF;
    setActiveAngleFactor(0.0f);
}

void GameObjectCheckpoint::update(int dt) {
    if (m_dishState == DISH_TURNING_OFF) {
        m_dishTimer = midp::Math::max(0, m_dishTimer - dt);
        if (m_dishTimer == 0) {
            m_dishState = DISH_OFF;
            setActiveAngleFactor(0.0f);
        } else {
            setActiveAngleFactor(m_dishTimer * 0.0013333333f);
        }
    } else if (m_dishState == DISH_TURNING_ON) {
        m_dishTimer = midp::Math::max(0, m_dishTimer - dt);
        if (m_dishTimer == 0) {
            m_dishState = DISH_ON;
            setActiveAngleFactor(1.0f);
        } else {
            setActiveAngleFactor((DISH_TURN_TIME - m_dishTimer) * 0.0013333333f);
        }
    }
    m_dish->setRenderingEnable(true);
}

void GameObjectCheckpoint::updateDishSelection() {
    if (AppEngine::getCanvas()->getSceneGame()->isCheckPointActive(this)) {
        if (m_dishState == DISH_TURNING_OFF) {
            m_dishState = DISH_TURNING_ON;
            m_dishTimer = DISH_TURN_TIME - m_dishTimer;
        } else if (m_dishState == DISH_OFF) {
            m_dishState = DISH_TURNING_ON;
            m_dishTimer = DISH_TURN_TIME;
        }
    } else {
        if (m_dishState == DISH_ON) {
            m_dishState = DISH_TURNING_OFF;
            m_dishTimer = DISH_TURN_TIME;
        } else if (m_dishState == DISH_TURNING_ON) {
            m_dishState = DISH_TURNING_OFF;
            m_dishTimer = DISH_TURN_TIME - m_dishTimer;
        }
    }
}

void GameObjectCheckpoint::resetCheckpoint() {
    if (AppEngine::getCanvas()->getSceneGame()->isCheckPointActive(this)) {
        m_dishState = DISH_ON;
        setActiveAngleFactor(1.0f);
    } else {
        m_dishState = DISH_OFF;
        setActiveAngleFactor(0.0f);
    }
}

void GameObjectCheckpoint::collidedWith(GameObject* other) {
    if (!other->isFlagSet(FLAG_PLAYER)) {
        return;
    }
    SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
    GameObjectPlayer* player = static_cast<GameObjectPlayer*>(other);
    GameScreen* screen = scene->getGameScreen(player->getPlayerIndex());
    if (screen->getCheckpointObject() == this) {
        return;
    }
    m_dishState = DISH_TURNING_ON;
    m_dishTimer = DISH_TURN_TIME;
    screen->setCheckpointObject(this);
}

GameObjectCheckpoint::~GameObjectCheckpoint() {}
