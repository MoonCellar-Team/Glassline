#pragma once

#include "game/GameObject.h"

class GameScreen;

class GameObjectEffectPickup : public GameObject {
public:
    static const int TYPE = 9;

    GameObjectEffectPickup(MEdgeMap* map, GameObject* item, GameScreen* screen);
    virtual ~GameObjectEffectPickup();

    virtual void update(int dt);

    midp::ReferenceCountedPointer<m3g::Node> m_model;
    midp::ReferenceCountedPointer<m3g::Node> m_ring0;
    midp::ReferenceCountedPointer<m3g::Node> m_ring1;
    int m_time;
    float m_angle;
    midp::ReferenceCountedPointer<GameScreen> m_screen;
};
