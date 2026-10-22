#include "game/AppEngine.h"

#include "game/ResourceManager.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/ReferenceCountedHardReference.h"

static const int RESOURCE_GAME_OBJECT_DATA = 198;

int GameObjectData::getFlags(int type) {
    return m_flags[type];
}

int GameObjectData::getCollidesWith(int type) {
    return m_collidesWith[type];
}

GameObjectData::GameObjectData() {}

GameObjectData::~GameObjectData() {}

void GameObjectData::loadData() {
    if (!(m_flags == NULL)) {
        return;
    }
    AppEngine::getCanvas();
    midp::DataInputStream* in =
        new midp::DataInputStream(ResourceManager::loadBinaryFile(RESOURCE_GAME_OBJECT_DATA));
    midp::ReferenceCountedHardReference ref(in);

    int count = in->readUnsignedShort();
    m_flags = midp::array<unsigned short>(count);
    m_collidesWith = midp::array<unsigned char>(count);
    for (int i = 0; i < count; i++) {
        m_flags[i] = in->readUnsignedShort();
        m_collidesWith[i] = in->readByte();
    }
    in->close();
}
