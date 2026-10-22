#pragma once

#include "game/GameObject.h"

namespace m3g {
class Node;
}

class GameObjectPlaneLight : public GameObject {
public:
    static const int TYPE = 8;

    GameObjectPlaneLight(MEdgeMap* map, float x, float y, float z);
    virtual ~GameObjectPlaneLight();

    virtual void resetLevel();
    virtual void update(int dt);

    m3g::Node* m_flash;
    int m_flashTimer;
};
