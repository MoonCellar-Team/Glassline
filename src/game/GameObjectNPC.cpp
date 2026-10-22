#include "game/GameObjectNPC.h"

#include "game/AnimationBlender.h"
#include "game/AnimPlayer3D.h"
#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/GameObjectPlayer.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"

float GameObjectNPC::DEFAULT_TURN_SPEED = midp::Math::PI + midp::Math::PI;

void GameObjectNPC::collidedWith(GameObject* other) {}

void GameObjectNPC::updateFacing(int dt) {
    float pi = midp::Math::PI;
    if (m_targetFacing == m_facing) {
        return;
    }
    float twoPi = pi + pi;
    float toTarget = m_targetFacing - m_facing;
    if (toTarget < 0) {
        toTarget += twoPi;
    } else if (toTarget > twoPi) {
        toTarget -= twoPi;
    }

    float dir = toTarget > pi ? -1.0f : 1.0f;
    m_facing = m_facing + m_turnSpeed * dir * (dt * 0.001f);
    if (m_facing < 0) {
        m_facing += twoPi;
    } else if (m_facing > twoPi) {
        m_facing -= twoPi;
    }

    float passed = m_facing - m_targetFacing;
    if (passed < 0) {
        passed += twoPi;
    } else if (passed > twoPi) {
        passed -= twoPi;
    }
    if ((dir > 0 && passed < pi) || (toTarget > pi && passed > pi)) {
        m_facing = m_targetFacing;
    }

    if (m_model != NULL) {
        m_model->setOrientation(m_facing * (180.0f / pi), 0.0f, -1.0f, 0.0f);
    }
}

void GameObjectNPC::updateFacePlayer(int dt) {
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    MathVector toPlayer(player->m_position.x - m_position.x, 0.0f, player->m_position.z - m_position.z);
    m_targetFacing = facingFromDirection(toPlayer.x, toPlayer.z);
}

void GameObjectNPC::startBlendedAnim(int anim, int channel, int flags) {
    m_animPlayer->startAnim(anim, flags);
    m_blender->setChannelWeight(channel, 1.0f, 3);
}

void GameObjectNPC::update(int dt) {
    m_time += dt;
    GameObject::update(dt);
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
}

GameObjectNPC::~GameObjectNPC() {}

GameObjectNPC::GameObjectNPC(MEdgeMap* map, int type, float x, float y, float z)
    : GameObjectRunner(map, type, x, y, z),
      m_npcState(NPC_STATE_DISABLED),
      m_time(0),
      m_facing(0),
      m_targetFacing(0),
      m_turnSpeed(DEFAULT_TURN_SPEED) {}
