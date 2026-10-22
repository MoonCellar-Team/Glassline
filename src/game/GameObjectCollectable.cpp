#include "game/GameObjectCollectable.h"

#include "game/AchievementData.h"
#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCommon.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectEffectPickup.h"
#include "game/GameObjectPlayer.h"
#include "game/GameScreen.h"
#include "game/LevelData.h"
#include "game/M3GAssets.h"
#include "game/MapPalette.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"
#include "game/SoundManager.h"
#include "m3g/Node.h"
#ifdef MEDECOMP_PORT
#include "port/net.h"
#endif

static const int FLAG_PLAYER = 1;

static const int MODEL_USER_ID = 0x10f;
static const int MODEL_USER_ID_ALT = 0x10e;

static const int SOUND_PICKUP = 70;
static const int SOUND_NEAR = 65;

static const float NEAR_DIST_SQ = 100.0f;

GameObjectCollectable::GameObjectCollectable(MEdgeMap* map, MapPalette& palette, int type, float x, float y,
                                             float z)
    : GameObject(map, type, x, y, z), m_nearSoundPlayed(false) {
    AppEngine* app = AppEngine::getCanvas();
    m_objectNode = palette.createUniqueNode(getType() == TYPE_BAG_ALT ? MODEL_USER_ID_ALT : MODEL_USER_ID);
    m_objectNode->setTranslation(x, y, z);
    M3GAssets::addNode(app->getSceneGame()->getM3GWorld(), m_objectNode);
    m_shape = new CollOrthoHexahedron(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);
    m_objectNodeCull = new GameObjectCulledGraphic(m_objectNode, getGlobalBounds());
    m_map->addCullObject(m_objectNodeCull);
    SoundManager& sound = app->getSoundManager();
    if (!sound.isEventLoaded(SOUND_PICKUP)) {
        sound.loadEvent(SOUND_PICKUP);
    }
    if (!sound.isEventLoaded(SOUND_NEAR)) {
        sound.loadEvent(SOUND_NEAR);
    }
}

void GameObjectCollectable::collidedWith(GameObject* other) {
    if (!other->isFlagSet(FLAG_PLAYER)) {
        return;
    }
    SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
    GameObjectPlayer* player = static_cast<GameObjectPlayer*>(other);
    GameScreen* screen = scene->getScreenForPlayer(player);
    m_map->addObject(new GameObjectEffectPickup(m_map, this, screen));
    m_map->removeObject(this);
    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_STORY) {
        scene->getGameScreen(0)->incNumCollectablesCollected();
    } else {
        player->incNumCollectablesCollected();
#ifdef MEDECOMP_PORT
        port::net::rivalsBagTaken(this);
#endif
    }
    AppEngine::getCanvas()->getAchievementData()->registerBag();
    AppEngine::getCanvas()->getSoundManager().playEvent(SOUND_PICKUP, 1.0f);
}

void GameObjectCollectable::update(int dt) {
    if (m_nearSoundPlayed) {
        return;
    }
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    if (player != NULL) {
        if (getDistanceToSq(player) >= NEAR_DIST_SQ) {
            return;
        }
        AppEngine::getCanvas()->getSoundManager().playEvent(SOUND_NEAR, 1.0f);
    }
    m_nearSoundPlayed = true;
}

GameObjectCollectable::~GameObjectCollectable() {}
