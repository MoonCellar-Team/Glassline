#pragma once

#include "game/GameObject.h"

class GameObjectPopupBox : public GameObject {
public:
    static const int TYPE = 16;

    GameObjectPopupBox(MEdgeMap* map, int text, float minX, float minY, float maxX, float maxY);
    virtual ~GameObjectPopupBox();

    virtual void resetCheckpoint();
    virtual void resetLevel();
    virtual void collidedWith(GameObject* other);

    int m_text;
    bool m_shown;
};
