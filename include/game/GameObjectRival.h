#pragma once

#include "game/GameObjectNPC.h"

class CollShape;
class Path;

class GameObjectRival : public GameObjectNPC {
public:
    static const NPCState STATE_SLIDE = (NPCState)3;
    static const NPCState STATE_RUN = (NPCState)5;
    static const NPCState STATE_JUMP = (NPCState)6;
    static const NPCState STATE_FALL = (NPCState)7;
    static const NPCState STATE_RAMP_UP = (NPCState)8;
    static const NPCState STATE_RAMP_DOWN = (NPCState)9;

    enum BrainState {
        BRAIN_NONE = 0,
        BRAIN_FOLLOW_PATH = 1,
        BRAIN_JUMP = 2,
        BRAIN_JUMP_TO = 3,
        BRAIN_SLIDE = 4,
        BRAIN_WALL_RUN = 5,
        BRAIN_CLIMB = 6,
        BRAIN_CLAMBER = 7,
        BRAIN_KNOCKED_DOWN = 8,
        BRAIN_GET_UP = 9,
    };

    enum AnimState {
        ANIM_NONE = 0,
        ANIM_RUN = 1,
        ANIM_JUMP = 2,
        ANIM_SLIDE = 4,
        ANIM_WALL_RUN = 5,
        ANIM_CLIMB = 6,
        ANIM_CLAMBER = 7,
        ANIM_RAMP_DOWN = 8,
        ANIM_RAMP_UP = 9,
        ANIM_KNOCKED_DOWN = 10,
        ANIM_GET_UP = 11,
    };

    enum NodeAction {
        NODE_RUN = 0,
        NODE_JUMP = 1,
        NODE_JUMP_TO = 2,
        NODE_CLIMB = 3,
        NODE_SLIDE = 4,
        NODE_WALL_RUN = 5,
    };

    static const int TYPE_RIVAL_GENERAL = 5;
    static const int TYPE_RIVAL_BOSS = 6;

    static const int UPDATE_RANGESQ;
    static const int NUM_TEST_ANIMS = 8;
    static const int testAnims[NUM_TEST_ANIMS];
    static const int testChannels[NUM_TEST_ANIMS];

    GameObjectRival(MEdgeMap* map, int type, int modelId, int blenderChannels, float x, float y, float z,
                    int pathIndex);
    virtual ~GameObjectRival();

    virtual void resetLevel();
    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);
    virtual void stateTransition(NPCState state);
    virtual bool shouldUpdate();

    bool onGround();
    void updateTestAnims(int dt);
    void updateState(int dt);
    void animStateTransition(AnimState state);
    void brainStateTransition(BrainState state);
    void processWallRunNode(MathVector& toNode);
    void processSlideNode(MathVector& toNode);
    void processClimbNode(MathVector& toNode);
    void processJumpToNode(MathVector& toNode);
    void processJumpNode(MathVector& toNode);
    void brainUpdateClamber(int dt);
    MathVector getVectorToNextPathNode();
    bool onReachedNode(MathVector& toNode);
    void brainUpdateClimb(int dt);
    void brainUpdateOnRails(int dt);
    bool hasPastPathNode();
    void brainUpdateFollowPath(int dt);
    void updatePosition(int dt);
    void updateBrain(int dt);

    NPCState m_state;
    BrainState m_brainState;
    BrainState m_savedBrainState;
    AnimState m_animState;
    int m_stateTime;
    float m_speed;
    bool m_combatStarted;
    midp::ReferenceCountedPointer<CollShape> m_standShape;
    midp::ReferenceCountedPointer<CollShape> m_slideShape;
    int m_testAnimIndex;
    int m_testAnimTimer;
    Path* m_path;
    int m_pathNode;
    midp::ReferenceCountedPointer<CollShape> m_rampShape;
    MathVector m_rampStart;
    MathVector m_rampEnd;
    int m_timesKnockedDown;
    int m_distToPlayerSq;
};
