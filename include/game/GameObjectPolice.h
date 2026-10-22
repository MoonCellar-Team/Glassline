#pragma once

#include "game/GameObjectNPC.h"

class GameObjectPlayer;
class Path;

class GameObjectPolice : public GameObjectNPC {
public:
    static const NPCState STATE_STAND = NPC_STATE_ACTIVE;
    static const NPCState STATE_CROUCH = (NPCState)3;
    static const NPCState STATE_PATROL = (NPCState)4;
    static const NPCState STATE_UNUSED = (NPCState)5;

    enum BrainState {
        BRAIN_IDLE = 0,
        BRAIN_PATROL = 1,
        BRAIN_ALERT = 2,
        BRAIN_AIM = 3,
        BRAIN_FIRE = 4,
        BRAIN_MELEE = 5,
        BRAIN_KNOCKED_DOWN = 6,
        BRAIN_GET_UP = 7,
        BRAIN_DISARMED = 8,
        BRAIN_MELEED_FROM_PRONE = 9,
        BRAIN_FALLEN = 10,
    };

    enum AnimState {
        ANIM_IDLE = 0,
        ANIM_PATROL = 1,
        ANIM_UNUSED = 2,
        ANIM_ALERT = 3,
        ANIM_ALERT_STAND = 4,
        ANIM_AIM_STAND = 5,
        ANIM_FIRE_STAND = 6,
        ANIM_ALERT_CROUCH = 7,
        ANIM_AIM_CROUCH = 8,
        ANIM_FIRE_CROUCH = 9,
        ANIM_MELEE = 10,
        ANIM_DISARMED = 11,
        ANIM_KNOCKED_FORWARD = 12,
        ANIM_KNOCKED_FORWARD_HIGH = 13,
        ANIM_KNOCKED_BACKWARD = 14,
        ANIM_KNOCKED_BACKWARD_HIGH = 15,
        ANIM_GET_UP_FORWARD = 16,
        ANIM_GET_UP_BACKWARD = 17,
        ANIM_MELEED_FROM_PRONE = 18,
        ANIM_FALL_FORWARD = 19,
        ANIM_FALL_BACKWARD = 20,
        ANIM_LAND = 21,
    };

    static const int TYPE_POLICE_LIGHT = 3;
    static const int TYPE_POLICE_RIOT = 4;

    static const int UPDATE_RANGESQ;
    static const int NUM_TEST_ANIMS = 15;
    static const int testAnims[NUM_TEST_ANIMS];
    static const int testChannels[NUM_TEST_ANIMS];

    struct Tuning {
        int shotDelay;
        int shotCheckInterval;
        int aimTime;
        int fireInterval;
        int burstChance;
        int postureCheckInterval;
        int postureChance;
        int postureDelay;
        int shotChance;
        int sightRangeAhead;
        int sightRangeBehind;
    };

    GameObjectPolice(MEdgeMap* map, int type, int modelId, int blenderChannels, float x, float y, float z,
                     int pathIndex);
    virtual ~GameObjectPolice();

    virtual void resetLevel();
    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);
    virtual void stateTransition(NPCState state);

    void meleedFromPlayerJumpingFromProne();
    bool isActive();
    bool facingPlayer();
    bool playerInRange();
    void facePlayer();
    void shoot();
    bool sightedPlayer();
    bool shouldTryShot(int dt);
    bool shouldSwitchPosture(int dt);
    void switchPosture();
    void updateTestAnims(int dt);
    void updateState(int dt);
    void playRandomGruntSound();
    void animStateTransition(AnimState state);
    void updateAnimState(int dt);
    void brainStateTransition(BrainState state);
    void patrolBrainUpdate(int dt);
    void fireBrainUpdate(int dt);
    void aimBrainUpdate(int dt);
    void alertBrainUpdate(int dt);
    void updateBrain(int dt);
    void updatePosition(int dt);

    NPCState m_state;
    BrainState m_brainState;
    AnimState m_animState;
    int m_stateTime;
    MathVector m_knockdownPos;
    int m_fallenTime;
    bool m_combatStarted;
    MathVector m_deceleration;
    int m_testAnimIndex;
    int m_testAnimTimer;
    int m_aimTimer;
    int m_postureTimer;
    int m_postureCheckTimer;
    int m_shotTimer;
    int m_shotCheckTimer;
    int m_fireTimer;
    int m_pathIndex;
    Path* m_path;
    int m_pathNode;
    float m_modelOffsetY;
    GameObjectPlayer* m_player;
    int m_distToPlayerSq;
    bool m_facePlayer;
    Tuning m_tuning;
};
