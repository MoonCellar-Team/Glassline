#include "game/GameObjectAmbientSound.h"

#include "game/AppEngine.h"
#include "game/SoundManager.h"

void GameObjectAmbientSound::updateSoundPos() {
    if (m_handle == -1) {
        return;
    }
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    if (sound.isHandlePlaying(m_handle)) {
        sound.setEventPosition(m_handle, m_position.x, m_position.y, m_position.z);
    } else {
        m_handle = -1;
    }
}

void GameObjectAmbientSound::update(int dt) {
    if (m_handle == -1) {
        SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
        m_handle = sound.playEvent(m_soundId, 1.0f);
        sound.setLoopCountEvent(m_handle, -1);
    }
    GameObject::update(dt);
    updateSoundPos();
}

GameObjectAmbientSound::~GameObjectAmbientSound() {
    if (m_handle != -1) {
        AppEngine::getCanvas()->getSoundManager().stopEvent(m_handle);
    }
}

GameObjectAmbientSound::GameObjectAmbientSound(MEdgeMap* map, int soundId, float x, float y, float z)
    : GameObject(map, TYPE, x, y, z), m_soundId(soundId), m_handle(-1) {
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    if (!sound.isEventLoaded(m_soundId)) {
        sound.loadEvent(m_soundId);
    }
}
