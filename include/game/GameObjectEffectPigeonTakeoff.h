#pragma once

#include "game/GameObject.h"
#include "runtime/midp/array.h"

class GameObjectEffectPigeonTakeoff_Pigeon;
namespace m3g {
class Group;
}

class GameObjectEffectPigeonTakeoff : public GameObject {
public:
    static const int TYPE = 22;

    GameObjectEffectPigeonTakeoff(MEdgeMap* map, float x, float y, float z);
    virtual ~GameObjectEffectPigeonTakeoff();

    virtual void resetCheckpoint();
    virtual void resetLevel();
    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);

    void pigeonTakenOff();

    m3g::Group* m_group;
    midp::array<GameObjectEffectPigeonTakeoff_Pigeon*> m_pigeons;
    bool m_scared;
    bool m_soundPlayed;
};
