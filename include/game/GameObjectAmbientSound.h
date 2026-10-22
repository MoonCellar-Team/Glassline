#pragma once

#include "game/GameObject.h"

class GameObjectAmbientSound : public GameObject {
public:
    static const int TYPE = 12;

    GameObjectAmbientSound(MEdgeMap* map, int soundId, float x, float y, float z);
    virtual ~GameObjectAmbientSound();

    virtual void update(int dt);

    void updateSoundPos();

    int m_soundId;
    int m_handle;
};
