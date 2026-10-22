#include "game/GameObjectBullet.h"

#include <math.h>

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectPlayer.h"
#include "game/M3GAssets.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "m3g/Node.h"
#include "m3g/Transform.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/array.h"

static const int FLAG_PLAYER = 1;

static const int MODEL_BULLET = 21;
static const int MODEL_FLAGS = 4;

static const int SOUND_POOL_BULLET_HIT = 4;
static const int SOUND_GROUP_BULLET_HIT = 0;

GameObjectBullet::GameObjectBullet(MEdgeMap* map, float x, float y, float z, const MathVector& velocity)
    : GameObject(map, TYPE, x, y, z), m_age(0) {
    AppEngine* app = AppEngine::getCanvas();
    M3GAssets* assets = AppEngine::getM3GAssets();
    m3g::Group* world = app->getSceneGame()->getM3GWorld();
    midp::array<float> unused;
    m_velocity.set(velocity);
    float yaw = midp::Math::toDegrees(atan2(velocity.z, velocity.x) - 1.5707964f);
    float pitch = midp::Math::toDegrees(atan2(velocity.z, velocity.y) - 1.5707964f);
    MathVector tail(velocity);
    tail *= -0.6f / tail.getLength();
    m_objectNode = assets->loadModel(MODEL_BULLET, MODEL_FLAGS);
    m3g::Transform transform;
    transform.postRotate(pitch, 1.0f, 0.0f, 0.0f);
    transform.postRotate(yaw, 0.0f, -1.0f, 0.0f);
    m_objectNode->setTranslation(x, y, z);
    m_objectNode->setTransform(&transform);
    m_objectNode->setScale(0.0f, 1.0f, 1.0f);
    M3GAssets::addNode(world, m_objectNode);
    m_shape = new CollOrthoHexahedron(-0.1f, -0.1f, -0.1f, 0.1f, 0.1f, 0.1f);
}

void GameObjectBullet::update(int dt) {
    m_age += dt;
    if (m_age >= LIFETIME) {
        m_map->removeObject(this);
        return;
    }
    m_objectNode->setScale((float)fmin(1.0, m_age * 0.005f), 1.0f, 1.0f);
    MathVector step(m_velocity);
    step *= dt * 0.001f;
    m_position += step;
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
    m_shape->setPosition(m_position);
    if (m_map->intersects(m_shape, 0)) {
        m_map->removeObject(this);
    } else {
        m_objectNode->setRenderingEnable(true);
    }
}

void GameObjectBullet::collidedWith(GameObject* other) {
    if (!other->isFlagSet(FLAG_PLAYER)) {
        return;
    }
    AppEngine* app = AppEngine::getCanvas();
    SoundManager& sound = app->getSoundManager();
    sound.playEvent(app->getSceneGame()->getSoundPoolManager().getRandomSoundEventID(SOUND_POOL_BULLET_HIT,
                                                                                    SOUND_GROUP_BULLET_HIT, -1),
                    1.0f);
    static_cast<GameObjectPlayer*>(other)->hurt(1.0f, false);
    m_map->removeObject(this);
}

GameObjectBullet::~GameObjectBullet() {}
