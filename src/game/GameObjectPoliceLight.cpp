#include "game/GameObjectPoliceLight.h"

#include "game/GameCommon.h"

static const int MODEL_LIGHT = 17;

GameObjectPoliceLight::GameObjectPoliceLight(MEdgeMap* map, float x, float y, float z, int pathIndex)
    : GameObjectPolice(map, TYPE_POLICE_LIGHT, MODEL_LIGHT, 1, x, y, z, pathIndex) {
    m_tuning.shotDelay = 300;
    m_tuning.shotCheckInterval = 300;
    m_tuning.aimTime = 300;
    m_tuning.fireInterval = 550;
    m_tuning.burstChance = 50;
    m_tuning.postureCheckInterval = 1000;
    m_tuning.postureChance = 10;
    m_tuning.postureDelay = 3000;
    m_tuning.shotChance = 80;
    m_tuning.sightRangeAhead = 7;
    m_tuning.sightRangeBehind = 1;
}

GameObjectPoliceLight::~GameObjectPoliceLight() {}
