#pragma once

#include "game/GameObject.h"

class GameObjectCutsceneTrigger : public GameObject {
public:
    static const int TYPE = 16;

    GameObjectCutsceneTrigger(MEdgeMap* map, int cutscene, float minX, float minY, float maxX, float maxY);
    virtual ~GameObjectCutsceneTrigger();

    virtual void resetCheckpoint();
    virtual void resetLevel();
    virtual void collidedWith(GameObject* other);

    int m_cutscene;
    bool m_played;
};
