#include "runtime/midp/Graphics.h"

#include "runtime/midp/Player.h"
#include "sdk/Foundation.h"

NSRecursiveLock* s_sharedAudioLock = NSRecursiveLock::alloc()->init();

namespace midp {

NSLock* s_Manager_lock = NSLock::alloc()->init();

util::VectorSE<Player> Manager_s_players;

util::VectorSE<Player> Manager_s_removeList;

GraphicsIPhoneGLES::GraphicsIPhoneGLES()
    : m_unk24(0), m_unk28(0), m_unk2c(0), m_unk30(0),
      m_originX(0), m_originY(0),
      m_xAxisX(1), m_xAxisY(0), m_yAxisX(0), m_yAxisY(1) {}

void GraphicsIPhoneGLES::clearViewport() {
    m_originX = 0;
    m_originY = 0;
    m_xAxisX = 1;
    m_xAxisY = 0;
    m_yAxisX = 0;
    m_yAxisY = 1;
}

void GraphicsIPhoneGLES::setViewport(int x, int y, int rotation) {
    m_originX = x;
    m_originY = y;
    switch (rotation & 3) {
    case 0:
        m_xAxisX = 1;
        m_xAxisY = 0;
        m_yAxisX = 0;
        m_yAxisY = 1;
        break;
    case 1:
        m_xAxisX = 0;
        m_xAxisY = 1;
        m_yAxisX = -1;
        m_yAxisY = 0;
        break;
    case 2:
        m_xAxisX = -1;
        m_xAxisY = 0;
        m_yAxisX = 0;
        m_yAxisY = -1;
        break;
    case 3:
        m_xAxisX = 0;
        m_xAxisY = -1;
        m_yAxisX = 1;
        m_yAxisY = 0;
        break;
    }
}

GraphicsIPhoneGLES::~GraphicsIPhoneGLES() {}

}

void releaseLock() {
    s_sharedAudioLock->unlock();
}

void aquireBusyLock() {
    s_sharedAudioLock->lock();
}
