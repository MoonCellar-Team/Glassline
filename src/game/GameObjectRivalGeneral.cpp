#include "game/GameObjectRivalGeneral.h"

static const int MODEL_GENERAL = 15;

GameObjectRivalGeneral::GameObjectRivalGeneral(MEdgeMap* map, float x, float y, float z, int pathIndex)
    : GameObjectRival(map, TYPE_RIVAL_GENERAL, MODEL_GENERAL, 0, x, y, z, pathIndex) {}

GameObjectRivalGeneral::~GameObjectRivalGeneral() {}
