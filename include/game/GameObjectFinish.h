#pragma once

#include "game/GameObject.h"

class GameObjectFinish : public GameObject {
public:
    static const int TYPE = 10;

    GameObjectFinish(MEdgeMap* map, float x, float y, float width, float height);
    virtual ~GameObjectFinish();

    virtual void collidedWith(GameObject* other);
};
