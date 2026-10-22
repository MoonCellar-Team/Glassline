#include "game/GameObjectPopupBox.h"

#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"

static const int FLAG_PLAYER = 1;

GameObjectPopupBox::GameObjectPopupBox(MEdgeMap* map, int text, float minX, float minY, float maxX, float maxY)
    : GameObject(map, TYPE), m_text(text), m_shown(false) {
    m_shape = new CollOrthoHexahedron(minX, minY, -1.0f, maxX, maxY, 1.0f);
}

void GameObjectPopupBox::resetCheckpoint() {}

GameObjectPopupBox::~GameObjectPopupBox() {}

void GameObjectPopupBox::resetLevel() {
    GameObject::resetLevel();
    m_shown = false;
}

void GameObjectPopupBox::collidedWith(GameObject* other) {
    if (!other->isFlagSet(FLAG_PLAYER)) {
        return;
    }
    if (AppEngine::getAchievementData()->registerPopupBox(m_text)) {
        m_shown = true;
    } else if (!m_shown && AppEngine::getCanvas()->getTutorialBoxes()) {
        AppEngine::getCanvas()->getSceneGame()->triggerPopupBox(m_text);
        m_shown = true;
    }
    m_map->removeObject(this);
}
