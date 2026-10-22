#include "game/GameObjectFinish.h"

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectPlayer.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"

static const int FLAG_PLAYER = 1;

GameObjectFinish::GameObjectFinish(MEdgeMap* map, float x, float y, float width, float height)
    : GameObject(map, TYPE, x, y, 0.0f) {
    m_shape = new CollOrthoHexahedron(0.0f, 0.0f, -0.5f, width, height, 0.5f);
}

void GameObjectFinish::collidedWith(GameObject* other) {
    if (!other->isFlagSet(FLAG_PLAYER)) {
        return;
    }
    static_cast<GameObjectPlayer*>(other)->setVictorious();
    AppEngine::getCanvas()->getSceneGame()->completeLevel();
    m_map->removeObject(this);
}

GameObjectFinish::~GameObjectFinish() {}
