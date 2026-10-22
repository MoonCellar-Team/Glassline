#include "game/GameObject.h"

#include "game/AnimPlayer3D.h"
#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectPlayer.h"
#include "game/M3GAssets.h"
#include "game/MathLine.h"
#include "game/MathOrthoBox.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"

const float GameObject::GRAVITY = -9.8f;

static const int TYPE_WITH_SMALL_BOX = 18;

GameObject::GameObject(MEdgeMap* map, int type)
    : m_map(map),
      m_type(type),
      m_id(-1),
      m_flags(0),
      m_static(false),
      m_shape(NULL),
      m_verticalAcceleration(0),
      m_objectNode(NULL),
      m_animPlayer(NULL),
      m_active(true),
      m_forward(1.0f, 0.0f, 0.0f),
      m_right(0.0f, 0.0f, 1.0f) {
    construct();
}

GameObject::GameObject(MEdgeMap* map, int type, float x, float y, float z)
    : m_map(map),
      m_type(type),
      m_id(-1),
      m_flags(0),
      m_static(false),
      m_position(x, y, z),
      m_shape(NULL),
      m_startPosition(x, y, z),
      m_verticalAcceleration(0),
      m_objectNode(NULL),
      m_animPlayer(NULL),
      m_active(true),
      m_forward(1.0f, 0.0f, 0.0f),
      m_right(0.0f, 0.0f, 1.0f) {
    construct();
}

GameObject::GameObject(MEdgeMap* map, int type, const MathVector& position)
    : m_map(map),
      m_type(type),
      m_id(-1),
      m_flags(0),
      m_static(false),
      m_position(position),
      m_shape(NULL),
      m_startPosition(position),
      m_verticalAcceleration(0),
      m_objectNode(NULL),
      m_animPlayer(NULL),
      m_active(true),
      m_forward(1.0f, 0.0f, 0.0f),
      m_right(0.0f, 0.0f, 1.0f) {
    construct();
}

GameObject::~GameObject() {
    M3GAssets::orphanNode(m_objectNode);
}

void GameObject::construct() {
    m_flags = AppEngine::getGameObjectData()->getFlags(m_type);
    if (m_type == TYPE_WITH_SMALL_BOX) {
        m_shape = new CollOrthoHexahedron(-0.2f, -0.2f, -0.2f, 0.2f, 0.2f, 0.2f);
    }
}

void GameObject::checkpointActivated() {}

void GameObject::resetCheckpoint() {
    resetLevel();
}

void GameObject::resetLevel() {
    m_active = true;
    m_position = m_startPosition;
    m_velocity.set(0.0f, 0.0f, 0.0f);
    m_prevVelocity = m_velocity;
    if (m_objectNodeCull != NULL) {
        m_objectNodeCull->setVisible(true);
    } else if (m_objectNode != NULL) {
        m_objectNode->setRenderingEnable(true);
    }
}

void GameObject::deactivate() {
    m_active = false;
    if (m_objectNodeCull != NULL) {
        m_objectNodeCull->setVisible(false);
    } else if (m_objectNode != NULL) {
        m_objectNode->setRenderingEnable(false);
    }
}

void GameObject::collidedWith(GameObject* ) {}

void GameObject::update(int dt) {
    m_prevVelocity = m_velocity;
    m_velocity.y += m_verticalAcceleration * (dt * 0.001f);
    if (m_objectNodeCull != NULL) {
        m_objectNodeCull->getBounds().setCoordinates(getGlobalBounds());
    }
}

void GameObject::updateAnimation(int ) {}

void GameObject::validateCollShape() {
    if (m_shape != NULL) {
        m_shape->setPosition(m_position);
    }
}

bool GameObject::intersectsMap() {
    m_shape->setPosition(m_position);
    return m_map->intersects(m_shape, 0);
}

const MathOrthoBox& GameObject::getGlobalBounds() {
    m_shape->setPosition(m_position);
    return m_shape->getBounds();
}

bool GameObject::checkLOSWithPlayer() {
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    MathLine line(m_position, player->m_position);
    line.m_direction -= line.m_origin;
    line.m_origin.y += 1.2f;
    float tMin = 0.0f;
    float tMax = 1.0f;
    return !m_map->intersects(line, 0, tMin, tMax);
}

float GameObject::getDistanceToSq(const GameObject* other) {
    MathVector d(m_position.x - other->m_position.x,
                 m_position.y - other->m_position.y,
                 m_position.z - other->m_position.z);
    return d.getLengthSq();
}

float GameObject::getLateralDistanceToSq(const GameObject* other) {
    MathVector d(m_position.x - other->m_position.x, 0.0f, m_position.z - other->m_position.z);
    return d.getLengthSq();
}

int GameObject::getType() {
    return m_type;
}

int GameObject::getID() {
    return m_id;
}

void GameObject::setID(int id) {
    m_id = id;
}

bool GameObject::isFlagSet(int flag) {
    return (m_flags & flag) != 0;
}

void GameObject::markObjectStatic() {
    m_static = true;
}

bool GameObject::isActive() {
    return m_active;
}

bool GameObject::isAnimating() {
    return m_animPlayer->isAnimating();
}

CollShape* GameObject::getShape() const {
    return m_shape;
}

m3g::Node* GameObject::getObjectNode() {
    return m_objectNode;
}

GameObjectCulledGraphic* GameObject::getObjectNodeCull() {
    return m_objectNodeCull;
}

MathVector GameObject::getForwardDirection() const {
    return m_forward;
}

MathVector GameObject::getRightDirection() const {
    return m_right;
}

const MathVector& GameObject::getVelocity() const {
    return m_velocity;
}

float GameObject::getForwardVelocity() const {
    return m_velocity.x;
}

void GameObject::setForwardVelocity(float v) {
    m_velocity.x = v;
}

float GameObject::getUpVelocity() const {
    return m_velocity.y;
}

float GameObject::getPrevUpVelocity() const {
    return m_prevVelocity.y;
}

void GameObject::setUpVelocity(float v) {
    m_velocity.y = v;
}

void GameObject::setVerticalAcceleration(float a) {
    m_verticalAcceleration = a;
}

void GameObject::setNoVelocity() {
    m_velocity.set(0.0f, 0.0f, 0.0f);
}
