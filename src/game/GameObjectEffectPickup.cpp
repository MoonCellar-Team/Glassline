#include "game/GameObjectEffectPickup.h"

#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/GameScreen.h"
#include "game/M3GAssets.h"
#include "game/MEdgeMap.h"
#include "game/QuadManager.h"
#include "game/SceneGame.h"
#include "m3g/Node.h"

static const int TIME_GROW = 300;
static const int TIME_END = 1200;

GameObjectEffectPickup::GameObjectEffectPickup(MEdgeMap* map, GameObject* item, GameScreen* screen)
    : GameObject(map, TYPE, item->m_position), m_time(0), m_angle(0.0f), m_screen(screen) {
    AppEngine* engine = AppEngine::getCanvas();
    M3GAssets* assets = AppEngine::getM3GAssets();
    QuadManager& quads = engine->getQuadManager();
    m3g::Group* world = engine->getSceneGame()->getM3GWorld();
    int scope = screen->getEffectScope();

    m_position.y += 0.5f;

    m3g::Group* group = new m3g::Group();
    m_objectNode = group;
    quads.addRawChild(group);

    m_model = static_cast<m3g::Node*>(item->getObjectNode()->duplicate());
    M3GAssets::addNode(world, m_model);
    m_model->setTranslation(m_position.x, m_position.y - 0.5f, m_position.z);

    m_ring0 = assets->loadModel(19, 4);
    m_ring0->setScope(scope);
    M3GAssets::addNode(group, m_ring0);

    m_ring1 = assets->loadModel(20, 4);
    m_ring1->setScope(scope);
    M3GAssets::addNode(group, m_ring1);

    update(0);
}

void GameObjectEffectPickup::update(int dt) {
    float seconds = dt * 0.001f;
    m_time += dt;
    if (m_time < TIME_GROW) {
        float t = m_time * -0.0033333334f + 1.0f;
        float scale = (1.0f - t * t * t) * 0.3f;
        m_objectNode->setScale(scale, scale, scale);
        m_model->setAlphaFactor(t);
    } else {
        if (m_time >= TIME_END) {
            m_map->removeObject(this);
            return;
        }
        float t = (m_time - TIME_GROW) * -0.0011111111f + 1.0f;
        float scale = (t * t * t + 1.0f) * 0.15f;
        m_objectNode->setScale(scale, scale, scale);
        m_ring0->setAlphaFactor(t);
        m_ring1->setAlphaFactor(t);
    }

    int x = 0;
    int y = 0;
    m_screen->getScreenLocalPosition(m_position, x, y);
    m_objectNode->setTranslation((float)x, (float)-y, 0.0f);

    m_angle += seconds * 180.0f;
    while (m_angle >= 360.0f) {
        m_angle -= 360.0f;
    }
    m_ring0->setOrientation(m_angle, 0.0f, 0.0f, 1.0f);
    m_ring1->setOrientation(-m_angle, 0.0f, 0.0f, 1.0f);
}

GameObjectEffectPickup::~GameObjectEffectPickup() {
    AppEngine::getCanvas()->getQuadManager().removeRawChild(m_objectNode);
    M3GAssets::orphanNode(m_model);
}
