#pragma once

#include "game/GameObject.h"

class MapPalette;

class GameObjectCollectable : public GameObject {
public:
    static const int TYPE_BAG_ALT = 19;

    GameObjectCollectable(MEdgeMap* map, MapPalette& palette, int type, float x, float y, float z);
    virtual ~GameObjectCollectable();

    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);

    bool m_nearSoundPlayed;
};
