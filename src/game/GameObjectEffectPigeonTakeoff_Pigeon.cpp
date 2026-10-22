#include "game/GameObjectEffectPigeonTakeoff_Pigeon.h"

#include "game/AnimPlayer3D.h"
#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectEffectPigeonTakeoff.h"
#include "game/M3GAssets.h"
#include "game/MathExt.h"
#include "game/MathOrthoBox.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"
#include "runtime/midp/Math.h"

static const int ANIM_IDLE = 147;
static const int ANIM_JUMP = 148;
static const int ANIM_FLY = 149;

static const float BOUNDS_HALF = 2.0f;

GameObjectEffectPigeonTakeoff_Pigeon::GameObjectEffectPigeonTakeoff_Pigeon(MEdgeMap* map,
                                                                           GameObjectEffectPigeonTakeoff* flock,
                                                                           m3g::Group* group, float x, float y,
                                                                           float z)
    : m_flock(flock),
      m_node(NULL),
      m_animPlayer(NULL),
      m_startPosition(x, y, z),
      m_position(x, y, z),
      m_takeoffSpeed(0.0f),
      m_flySpeed(0.0f),
      m_startAngle(0.0f),
      m_turnTime(0),
      m_targetAngle(0.0f),
      m_state(STATE_IDLE),
      m_stateTime(0),
      m_delay(0) {
    AppEngine* engine = AppEngine::getCanvas();
    m_node = AppEngine::getM3GAssets()->loadModel(22, 4);
    midp::INCREF(m_node);
    M3GAssets::addNode(group, m_node);

    m_animPlayer = new AnimPlayer3D(engine->getAnimationManager3D());
    m_animPlayer->setNode(m_node);
    m_animPlayer->startAnim(ANIM_IDLE, AnimPlayer3D::START_RESTART | AnimPlayer3D::START_LOOP);
    int time = engine->rand(0, m_animPlayer->getAnimDuration() - 1);
    m_animPlayer->setAnimTime(time);

    m_startPosition.x += engine->randFloat(-0.2f, 0.2f);
    m_startPosition.z += engine->randFloat(-0.2f, 0.2f);
    m_position = m_startPosition;
    m_node->setTranslation(m_startPosition.x, m_startPosition.y, m_startPosition.z);
    m_node->setScale(0.01f, 0.01f, 0.01f);
    m_startAngle = (float)engine->rand(0, 350);
    m_node->setOrientation(m_startAngle, 0.0f, 1.0f, 0.0f);

    MathOrthoBox bounds(m_position.x - BOUNDS_HALF, m_position.y - BOUNDS_HALF, m_position.z - BOUNDS_HALF,
                        m_position.x + BOUNDS_HALF, m_position.y + BOUNDS_HALF, m_position.z + BOUNDS_HALF);
    m_cull = new GameObjectCulledGraphic(m_node, bounds);
    map->addCullObject(m_cull);
}

void GameObjectEffectPigeonTakeoff_Pigeon::update(int dt) {
    if (m_state == STATE_GONE) {
        return;
    }
    m_stateTime += dt;

    if (m_state >= STATE_JUMP && m_state <= STATE_FLY && m_turnTime < 1000) {
        m_turnTime = midp::Math::min(1000, m_turnTime + dt);
        float t = m_turnTime * 0.001f;
        m_node->setOrientation(m_startAngle + t * t * (m_targetAngle - m_startAngle), 0.0f, 1.0f, 0.0f);
    }

    switch (m_state) {
    case STATE_SCARED:
        if (m_stateTime >= m_delay) {
            m_animPlayer->startAnim(ANIM_JUMP, AnimPlayer3D::START_RESTART);
            m_state = STATE_JUMP;
        }
        break;

    case STATE_JUMP: {
        AnimPlayer3D* anim = m_animPlayer;
        if (!anim->isAnimating()) {
            anim->startAnim(ANIM_FLY, AnimPlayer3D::START_RESTART | AnimPlayer3D::START_LOOP);
            m_state = STATE_TAKEOFF;
            m_stateTime = 0;
            m_flock->pigeonTakenOff();
        }
        MathVector step(m_direction);
        step *= dt * 0.001f;
        m_position.x += step.x;
        m_position.z += step.z;
        m_node->setTranslation(m_position.x, m_position.y, m_position.z);
        break;
    }

    case STATE_TAKEOFF: {
        if (m_stateTime >= m_delay) {
            m_state = STATE_FLY;
            m_stateTime = 0;
            break;
        }
        float t = (float)m_stateTime / (float)m_delay;
        float speed = m_takeoffSpeed + t * t * (m_flySpeed - m_takeoffSpeed);
        MathVector step(m_direction);
        step *= dt * speed * 0.001f;
        m_position += step;
        m_node->setTranslation(m_position.x, m_position.y, m_position.z);
        break;
    }

    case STATE_FLY: {
        if ((float)m_stateTime > 60000.0f) {
            m_state = STATE_GONE;
            m_cull->setVisible(false);
        }
        MathVector step(m_direction);
        step *= m_flySpeed * dt * 0.001f;
        m_position += step;
        m_node->setTranslation(m_position.x, m_position.y, m_position.z);
        break;
    }

    default:
        break;
    }

    m_cull->getBounds().setCoordinates(m_position.x - BOUNDS_HALF, m_position.y - BOUNDS_HALF,
                                       m_position.z - BOUNDS_HALF, m_position.x + BOUNDS_HALF,
                                       m_position.y + BOUNDS_HALF, m_position.z + BOUNDS_HALF);
    if (!m_cull->isFullyCulled()) {
        m_node->setRenderingEnable(true);
        m_animPlayer->updateAnim(dt);
    }
}

void GameObjectEffectPigeonTakeoff_Pigeon::takeoff(const MathVector& from) {
    AppEngine* engine = AppEngine::getCanvas();
    MathVector away(m_startPosition);
    away -= from;
    float dist = away.getLength();
    m_direction.x = away.x * engine->randFloat(1.0f, 4.0f);
    m_direction.y = engine->randFloat(0.1f, 1.0f) * dist;
    m_direction.z = away.z * engine->randFloat(1.0f, 4.0f);
    m_direction.normalise();

    m_delay = engine->rand(0, 500);
    m_takeoffSpeed = engine->randFloat(3.0f, 6.0f);
    m_flySpeed = engine->randFloat(8.0f, 12.0f);
    float k = midp::Math::max(0.0f, midp::Math::min(1.0f, (away.getLengthSq() - 30.0f) * 0.06666667f));
    m_delay = (int)(k * 200.0f);
    m_stateTime = 0;

    m_targetAngle = midp::Math::toDegrees(MathExt::atan2(m_direction.x, m_direction.z));
    if (midp::Math::abs(m_startAngle - m_targetAngle) > 180.0f) {
        if (m_startAngle < m_targetAngle) {
            m_targetAngle -= 360.0f;
        } else {
            m_targetAngle += 360.0f;
        }
    }
    m_state = STATE_SCARED;
}

void GameObjectEffectPigeonTakeoff_Pigeon::reset() {
    m_position = m_startPosition;
    m_turnTime = 0;
    m_cull->setVisible(true);
    m_node->setTranslation(m_startPosition.x, m_startPosition.y, m_startPosition.z);
    m_node->setOrientation(m_startAngle, 0.0f, 1.0f, 0.0f);
    m_animPlayer->startAnim(ANIM_IDLE, AnimPlayer3D::START_RESTART | AnimPlayer3D::START_LOOP);
    m_state = STATE_IDLE;
}

GameObjectEffectPigeonTakeoff_Pigeon::~GameObjectEffectPigeonTakeoff_Pigeon() {
    midp::DECREF(m_node);
    m_node = NULL;
    AppEngine::getCanvas()->getSceneGame()->getMap().removeCullObject(m_cull);
}
