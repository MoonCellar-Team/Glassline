#include "game/GameObjectPoliceRiot.h"

#include "game/GameCommon.h"

static const int MODEL_RIOT = 18;

GameObjectPoliceRiot::GameObjectPoliceRiot(MEdgeMap* map, float x, float y, float z, int pathIndex)
    : GameObjectPolice(map, TYPE_POLICE_RIOT, MODEL_RIOT, 1, x, y, z, pathIndex) {
    m_tuning.shotDelay = 200;
    m_tuning.shotCheckInterval = 200;
    m_tuning.aimTime = 200;
    m_tuning.fireInterval = 450;
    m_tuning.burstChance = 75;
    m_tuning.postureCheckInterval = 1000;
    m_tuning.postureChance = 10;
    m_tuning.postureDelay = 3000;
    m_tuning.shotChance = 90;
    m_tuning.sightRangeAhead = 15;
    m_tuning.sightRangeBehind = 7;
}

GameObjectPoliceRiot::~GameObjectPoliceRiot() {}
