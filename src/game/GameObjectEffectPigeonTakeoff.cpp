#include "game/GameObjectEffectPigeonTakeoff.h"

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameObjectEffectPigeonTakeoff_Pigeon.h"
#include "game/M3GAssets.h"
#include "game/SceneGame.h"
#include "game/SoundManager.h"
#include "m3g/Node.h"

typedef GameObjectEffectPigeonTakeoff_Pigeon Pigeon;

static const int SOUND_PIGEONS = 79;

void GameObjectEffectPigeonTakeoff::pigeonTakenOff() {
    if (m_soundPlayed) {
        return;
    }
    AppEngine::getCanvas()->getSoundManager().playEvent(SOUND_PIGEONS, 1.0f);
    m_soundPlayed = true;
}

GameObjectEffectPigeonTakeoff::~GameObjectEffectPigeonTakeoff() {
    for (int i = 0; i != m_pigeons.m_length; i++) {
        midp::DECREF(m_pigeons[i]);
    }
    m_pigeons = midp::array<Pigeon*>();
    M3GAssets::orphanNode(m_group);
    midp::DECREF(m_group);
    m_group = NULL;
}

void GameObjectEffectPigeonTakeoff::resetCheckpoint() {
    if (!m_scared) {
        return;
    }
    for (int i = 0; i != m_pigeons.m_length; i++) {
        m_pigeons[i]->reset();
    }
    m_scared = false;
}

void GameObjectEffectPigeonTakeoff::resetLevel() {
    if (!m_scared) {
        return;
    }
    for (int i = 0; i != m_pigeons.m_length; i++) {
        m_pigeons[i]->reset();
    }
    m_scared = false;
}

void GameObjectEffectPigeonTakeoff::collidedWith(GameObject* other) {
    if (m_scared) {
        return;
    }
    for (int i = 0; i != m_pigeons.m_length; i++) {
        m_pigeons[i]->takeoff(other->m_position);
    }
    m_scared = true;
}

void GameObjectEffectPigeonTakeoff::update(int dt) {
    for (int i = 0; i != m_pigeons.m_length; i++) {
        m_pigeons[i]->update(dt);
    }
}

GameObjectEffectPigeonTakeoff::GameObjectEffectPigeonTakeoff(MEdgeMap* map, float x, float y, float z)
    : GameObject(map, TYPE, x, y, z), m_group(NULL), m_scared(false), m_soundPlayed(false) {
    AppEngine* engine = AppEngine::getCanvas();
    m3g::Group* world = engine->getSceneGame()->getM3GWorld();
    m_shape = new CollOrthoHexahedron(-6.0f, -6.0f, -6.0f, 6.0f, 6.0f, 6.0f);
    m_group = new m3g::Group();
    midp::INCREF(m_group);
    M3GAssets::addNode(world, m_group);

    int count = engine->rand(4, 6);
    m_pigeons = midp::array<Pigeon*>(count);
    switch (count) {
    case 4:
        m_pigeons[0] = new Pigeon(map, this, m_group, x - 0.5f, y, z - 0.5f);
        m_pigeons[1] = new Pigeon(map, this, m_group, x - 0.5f, y, z + 0.5f);
        m_pigeons[2] = new Pigeon(map, this, m_group, x + 0.5f, y, z - 0.5f);
        m_pigeons[3] = new Pigeon(map, this, m_group, x + 0.5f, y, z + 0.5f);
        break;
    case 5:
        m_pigeons[0] = new Pigeon(map, this, m_group, x, y, z);
        m_pigeons[1] = new Pigeon(map, this, m_group, x - 0.6, y, z - 0.6);
        m_pigeons[2] = new Pigeon(map, this, m_group, x - 0.6, y, z + 0.6);
        m_pigeons[3] = new Pigeon(map, this, m_group, x + 0.6, y, z - 0.6);
        m_pigeons[4] = new Pigeon(map, this, m_group, x + 0.6, y, z + 0.6);
        break;
    case 6:
        m_pigeons[0] = new Pigeon(map, this, m_group, x - 0.5f, y, z - 0.5f);
        m_pigeons[1] = new Pigeon(map, this, m_group, x - 0.5f, y, z + 0.5f);
        m_pigeons[2] = new Pigeon(map, this, m_group, x + 0.5f, y, z - 0.5f);
        m_pigeons[3] = new Pigeon(map, this, m_group, x + 0.5f, y, z + 0.5f);
        m_pigeons[4] = new Pigeon(map, this, m_group, x - 1.0f, y, z);
        m_pigeons[5] = new Pigeon(map, this, m_group, x + 1.0f, y, z);
        break;
    }
    for (int i = 0; i != count; i++) {
        midp::INCREF(m_pigeons[i]);
    }
}
