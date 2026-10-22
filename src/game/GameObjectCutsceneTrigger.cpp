#include "game/GameObjectCutsceneTrigger.h"

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"

static const int FLAG_PLAYER = 1;

void GameObjectCutsceneTrigger::resetCheckpoint() {}

void GameObjectCutsceneTrigger::resetLevel() {
    GameObject::resetLevel();
    m_played = false;
}

void GameObjectCutsceneTrigger::collidedWith(GameObject* other) {
    if (!other->isFlagSet(FLAG_PLAYER)) {
        return;
    }
    AppEngine* app = AppEngine::getCanvas();
    if (!m_played) {
        app->getSceneGame()->playCutscene(m_cutscene);
        m_played = true;
    }
    m_map->removeObject(this);
}

GameObjectCutsceneTrigger::GameObjectCutsceneTrigger(MEdgeMap* map, int cutscene, float minX, float minY,
                                                     float maxX, float maxY)
    : GameObject(map, TYPE), m_cutscene(cutscene), m_played(false) {
    m_shape = new CollOrthoHexahedron(minX, minY, -1.0f, maxX, maxY, 1.0f);
}

GameObjectCutsceneTrigger::~GameObjectCutsceneTrigger() {}
