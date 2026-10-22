#pragma once

#include "game/GameObject.h"

class GameObjectMusicCue : public GameObject {
public:
    static const int TYPE = 12;

    GameObjectMusicCue(MEdgeMap* map, int musicId, float x, float y, float z);
    virtual ~GameObjectMusicCue();

    virtual void resetLevel();
    virtual void update(int dt);

    int m_musicId;
    bool m_triggered;
};
