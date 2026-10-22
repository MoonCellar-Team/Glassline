#include "game/GameObjectPlaneLight.h"

#include "game/AppEngine.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/M3GAssets.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"

static const int MODEL_PLANE_LIGHT = 23;
static const int MODEL_FLAGS = 4;

static const int FLASH_PERIOD = 2000;
static const int FLASH_TIME = 80;

void GameObjectPlaneLight::update(int dt) {
    m_position += m_velocity * (dt * 0.001f);
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
    m_flashTimer -= dt;
    if (m_flashTimer > 0) {
        m_flash->setRenderingEnable(m_flashTimer <= FLASH_TIME);
    } else {
        m_flashTimer = FLASH_PERIOD;
    }
}

void GameObjectPlaneLight::resetLevel() {
    m_position = m_startPosition;
    m_prevVelocity = m_velocity;
    if (m_objectNodeCull != NULL) {
        m_objectNodeCull->setVisible(true);
    } else if (m_objectNode != NULL) {
        m_objectNode->setRenderingEnable(true);
    }
}

GameObjectPlaneLight::GameObjectPlaneLight(MEdgeMap* map, float x, float y, float z)
    : GameObject(map, TYPE, x, y, z), m_flash(NULL), m_flashTimer(0) {
    AppEngine* app = AppEngine::getCanvas();
    M3GAssets* assets = AppEngine::getM3GAssets();
    m3g::Group* world = app->getSceneGame()->getM3GWorld();
    m_flash = assets->loadModel(MODEL_PLANE_LIGHT, MODEL_FLAGS);
    m3g::Node* light = assets->loadModel(MODEL_PLANE_LIGHT, MODEL_FLAGS);
    light->setScale(3.0f, 3.0f, 3.0f);
    m_flash->setScale(10.0f, 10.0f, 10.0f);
    m_flash->setRenderingEnable(false);
    m3g::Group* group = new m3g::Group();
    group->addChild(light);
    group->addChild(m_flash);
    world->addChild(group);
    m_objectNode = group;
    if (AppEngine::getCanvas()->randPercent() < 50) {
        m_velocity = MathVector(3.0f, 0.0f, 0.0f);
    } else {
        m_velocity = MathVector(-3.0f, 0.0f, 0.0f);
    }
    m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
    m_flashTimer = AppEngine::getCanvas()->rand(0, FLASH_PERIOD);
}

GameObjectPlaneLight::~GameObjectPlaneLight() {}
