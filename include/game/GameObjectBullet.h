#pragma once

#include "game/GameObject.h"

class GameObjectBullet : public GameObject {
public:
    static const int TYPE = 21;

    static const int LIFETIME = 4000;

    GameObjectBullet(MEdgeMap* map, float x, float y, float z, const MathVector& velocity);
    virtual ~GameObjectBullet();

    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);

    int m_age;
};
