#include "game/GameObjectMusicCue.h"

#include "game/AppEngine.h"
#include "game/BGMusic.h"
#include "game/GameObjectPlayer.h"
#include "game/SceneGame.h"

void GameObjectMusicCue::resetLevel() {
    m_triggered = false;
}

void GameObjectMusicCue::update(int dt) {
    GameObject::update(dt);
    if (m_triggered) {
        return;
    }
    GameObjectPlayer* player = AppEngine::getCanvas()->getSceneGame()->getSinglePlayerObject();
    float dx = player->m_position.x - m_position.x;
    float dy = player->m_position.y - m_position.y;
    float dz = player->m_position.z - m_position.z;
    if (dx * dx + dy * dy + dz * dz >= 100.0f) {
        return;
    }
    AppEngine::getCanvas()->getBGMusic()->playMusic(m_musicId, BGMusic::PLAY_LOOP);
    m_triggered = true;
}

GameObjectMusicCue::GameObjectMusicCue(MEdgeMap* map, int musicId, float x, float y, float z)
    : GameObject(map, TYPE, x, y, z), m_musicId(musicId), m_triggered(false) {}

GameObjectMusicCue::~GameObjectMusicCue() {}
