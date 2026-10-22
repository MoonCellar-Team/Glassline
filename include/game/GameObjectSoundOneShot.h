#pragma once

#include "game/GameObject.h"

class GameObjectSoundOneShot : public GameObject {
public:
    static const int TYPE = 14;

    GameObjectSoundOneShot(MEdgeMap* map, int soundId, float x, float y, float z);
    virtual ~GameObjectSoundOneShot();

    virtual void resetLevel();
    virtual void update(int dt);

    int m_soundId;
    bool m_played;
};
