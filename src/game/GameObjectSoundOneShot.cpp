#include "game/GameObjectSoundOneShot.h"

#include "game/AppEngine.h"
#include "game/GameObjectPlayer.h"
#include "game/SceneGame.h"
#include "game/SoundManager.h"

void GameObjectSoundOneShot::resetLevel() {
    m_played = false;
}

void GameObjectSoundOneShot::update(int dt) {
    GameObject::update(dt);
    AppEngine* app = AppEngine::getCanvas();
    GameObjectPlayer* player = app->getSceneGame()->getSinglePlayerObject();
    if (m_played || player == NULL) {
        return;
    }
    if ((player->m_position - m_position).getLengthSq() > 100.0f) {
        return;
    }
    SoundManager& sound = app->getSoundManager();
    int handle = sound.playEvent(m_soundId, 1.0f);
    sound.setLoopCountEvent(handle, 1);
    m_played = true;
}

GameObjectSoundOneShot::~GameObjectSoundOneShot() {
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    if (sound.isEventLoaded(m_soundId)) {
        sound.unloadEvent(m_soundId);
    }
}

GameObjectSoundOneShot::GameObjectSoundOneShot(MEdgeMap* map, int soundId, float x, float y, float z)
    : GameObject(map, TYPE, x, y, z), m_soundId(soundId), m_played(false) {
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    if (!sound.isEventLoaded(m_soundId)) {
        sound.loadEvent(m_soundId);
    }
}
