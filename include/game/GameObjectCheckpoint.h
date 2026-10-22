#pragma once

#include "game/GameObject.h"
#include "game/GameObjectRunner.h"

class ModelSet;

class GameObjectCheckpoint : public GameObject {
public:
    static const int TYPE = 15;

    enum DishState {
        DISH_OFF = 0,
        DISH_TURNING_ON = 1,
        DISH_ON = 2,
        DISH_TURNING_OFF = 3,
    };

    static const int DISH_TURN_TIME = 750;
    static const int WIDE_DISH_USER_ID = 202;

    GameObjectCheckpoint(MEdgeMap* map, const ModelSet* modelSet, float minX, float minY, float maxX, float maxY,
                         m3g::Node* graphic, bool facingRight);
    virtual ~GameObjectCheckpoint();

    virtual void resetCheckpoint();
    virtual void resetLevel();
    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);

    GameObjectRunner::FacingDir getPlayerFacingDir();
    void setActiveAngleFactor(float factor);
    void updateDishSelection();

    int m_dishUserId;
    m3g::Node* m_dish;
    float m_offAngle;
    float m_onAngle;
    DishState m_dishState;
    int m_dishTimer;
    GameObjectRunner::FacingDir m_playerFacingDir;
};
