#pragma once

#include "game/GameObjectRival.h"

class GameObjectRivalBoss : public GameObjectRival {
public:
    GameObjectRivalBoss(MEdgeMap* map, float x, float y, float z, int pathIndex);
    virtual ~GameObjectRivalBoss();

    virtual void resetLevel();
    virtual void update(int dt);
    virtual bool shouldUpdate();

    bool m_notFinalFight;
    bool m_slideDecided;
    int m_slideTimer;
    bool m_playerSeen;
};
