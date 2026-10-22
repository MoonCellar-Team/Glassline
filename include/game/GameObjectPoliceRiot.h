#pragma once

#include "game/GameObjectPolice.h"

class GameObjectPoliceRiot : public GameObjectPolice {
public:
    GameObjectPoliceRiot(MEdgeMap* map, float x, float y, float z, int pathIndex);
    virtual ~GameObjectPoliceRiot();
};
