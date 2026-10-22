#pragma once

#include <math.h>

#include "game/GameObjectRunner.h"
#include "runtime/midp/Math.h"

class GameObjectNPC : public GameObjectRunner {
public:
    enum NPCState {
        NPC_STATE_START = 0,
        NPC_STATE_TEST_ANIMS = 1,
        NPC_STATE_ACTIVE = 2,
        NPC_STATE_DISABLED = 10,
    };

    static float DEFAULT_TURN_SPEED;

    GameObjectNPC(MEdgeMap* map, int type, float x, float y, float z);
    virtual ~GameObjectNPC();

    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);
    virtual void stateTransition(NPCState state) = 0;

    void updateFacing(int dt);
    void updateFacePlayer(int dt);
    void startBlendedAnim(int anim, int channel, int flags);

    static float facingFromDirection(float dx, float dz);

    int m_npcState;
    int m_time;
    float m_facing;
    float m_targetFacing;
    float m_turnSpeed;
};

inline float GameObjectNPC::facingFromDirection(float dx, float dz) {
    static const float HALF_PI = 1.57079637f;
    float facing = (float)(atan2(dz, dx) - HALF_PI);
    float twoPi = midp::Math::PI + midp::Math::PI;
    if (facing < 0) {
        facing += twoPi;
    } else if (facing > twoPi) {
        facing -= twoPi;
    }
    return facing;
}
