#pragma once

#include "game/GameObjectPolice.h"

class GameObjectPoliceLight : public GameObjectPolice {
public:
    GameObjectPoliceLight(MEdgeMap* map, float x, float y, float z, int pathIndex);
    virtual ~GameObjectPoliceLight();
};
