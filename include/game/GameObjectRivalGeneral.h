#pragma once

#include "game/GameObjectRival.h"

class GameObjectRivalGeneral : public GameObjectRival {
public:
    GameObjectRivalGeneral(MEdgeMap* map, float x, float y, float z, int pathIndex);
    virtual ~GameObjectRivalGeneral();
};
