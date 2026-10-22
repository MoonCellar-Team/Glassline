#pragma once

#include "game/MathVector.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class AnimPlayer3D;
class GameObjectCulledGraphic;
class GameObjectEffectPigeonTakeoff;
class MEdgeMap;
namespace m3g {
class Group;
class Node;
}

class GameObjectEffectPigeonTakeoff_Pigeon : public midp::ReferenceCounted {
public:
    enum State {
        STATE_IDLE = 0,
        STATE_SCARED = 1,
        STATE_JUMP = 2,
        STATE_TAKEOFF = 3,
        STATE_FLY = 4,
        STATE_GONE = 5,
    };

    GameObjectEffectPigeonTakeoff_Pigeon(MEdgeMap* map, GameObjectEffectPigeonTakeoff* flock, m3g::Group* group,
                                         float x, float y, float z);
    virtual ~GameObjectEffectPigeonTakeoff_Pigeon();

    void update(int dt);
    void takeoff(const MathVector& from);
    void reset();

    GameObjectEffectPigeonTakeoff* m_flock;
    m3g::Node* m_node;
    midp::ReferenceCountedPointer<GameObjectCulledGraphic> m_cull;
    midp::ReferenceCountedPointer<AnimPlayer3D> m_animPlayer;
    MathVector m_startPosition;
    MathVector m_position;
    MathVector m_direction;
    float m_takeoffSpeed;
    float m_flySpeed;
    float m_startAngle;
    int m_turnTime;
    float m_targetAngle;
    State m_state;
    int m_stateTime;
    int m_delay;
};
