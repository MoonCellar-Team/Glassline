#pragma once

#include "game/MathVector.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"

class AnimPlayer3D;
class CollShape;
class GameObjectCulledGraphic;
class MathOrthoBox;
class MEdgeMap;
namespace m3g {
class Node;
}

class GameObject : public midp::ReferenceCounted {
public:
    static const float GRAVITY;

    GameObject(MEdgeMap* map, int type);
    GameObject(MEdgeMap* map, int type, float x, float y, float z);
    GameObject(MEdgeMap* map, int type, const MathVector& position);
    virtual ~GameObject();

    virtual void checkpointActivated();
    virtual void resetCheckpoint();
    virtual void resetLevel();
    virtual void validateCollShape();
    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);
    virtual void updateAnimation(int dt);

    void construct();
    void deactivate();

    int getType();
    int getID();
    void setID(int id);
    bool isFlagSet(int flag);
    void markObjectStatic();
    bool isActive();
    bool isAnimating();

    CollShape* getShape() const;
    const MathOrthoBox& getGlobalBounds();
    bool intersectsMap();
    bool checkLOSWithPlayer();
    m3g::Node* getObjectNode();
    GameObjectCulledGraphic* getObjectNodeCull();

    float getDistanceToSq(const GameObject* other);
    float getLateralDistanceToSq(const GameObject* other);
    MathVector getForwardDirection() const;
    MathVector getRightDirection() const;

    const MathVector& getVelocity() const;
    float getForwardVelocity() const;
    void setForwardVelocity(float v);
    float getUpVelocity() const;
    float getPrevUpVelocity() const;
    void setUpVelocity(float v);
    void setVerticalAcceleration(float a);
    void setNoVelocity();

    MEdgeMap* m_map;
    int m_type;
    int m_id;
    int m_flags;
    bool m_static;
    MathVector m_position;
    MathVector m_velocity;
    MathVector m_prevVelocity;
    midp::ReferenceCountedPointer<CollShape> m_shape;
    MathVector m_startPosition;
    float m_verticalAcceleration;
    midp::ReferenceCountedPointer<m3g::Node> m_objectNode;
    midp::ReferenceCountedPointer<GameObjectCulledGraphic> m_objectNodeCull;
    midp::ReferenceCountedPointer<AnimPlayer3D> m_animPlayer;
    bool m_active;
    MathVector m_forward;
    MathVector m_right;
};
