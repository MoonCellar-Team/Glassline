#include "runtime/midp/OpenGLESManager.h"

#include "runtime/midp/GLES.h"

namespace midp {

GLES* OpenGLESManager::m_gl;
int OpenGLESManager::s_texturesToBeDeletedPos;
int OpenGLESManager::s_texturesToBeDeleted[128];
int OpenGLESManager::s_buffersToBeDeletedPos;
int OpenGLESManager::s_buffersToBeDeleted[128];

void OpenGLESManager::setGLES(GLES* gl) {
    m_gl = gl;
}

GLES11* OpenGLESManager::getGLES() {
    return reinterpret_cast<GLES11*>(m_gl);
}

void OpenGLESManager::removeTexture(int texture) {
    s_texturesToBeDeleted[s_texturesToBeDeletedPos] = texture;
    s_texturesToBeDeletedPos += 1;
}

void OpenGLESManager::removeBuffer(int buffer) {
    if (buffer == 0)
        return;
    s_buffersToBeDeleted[s_buffersToBeDeletedPos] = buffer;
    s_buffersToBeDeletedPos += 1;
}

void OpenGLESManager::flushDeletedTextures() {
    int count = s_texturesToBeDeletedPos;
    if (count >= 1) {
        for (int i = count; i != 0; --i) {
            unsigned int id = (unsigned int)s_texturesToBeDeleted[i - 1];
            s_texturesToBeDeleted[i - 1] = 0;
            if (id != 0)
                m_gl->glDeleteTextures(1, &id);
        }
    }
    if (count <= 127) {
        for (int i = 0; i < 128 - count; ++i)
            s_texturesToBeDeleted[i] = s_texturesToBeDeleted[i + count];
    }
    s_texturesToBeDeletedPos -= count;

    count = s_buffersToBeDeletedPos;
    if (count >= 1) {
        for (int i = count; i != 0; --i) {
            int id = s_buffersToBeDeleted[i - 1];
            s_buffersToBeDeleted[i - 1] = 0;
            if (id != 0)
                m_gl->glDeleteBuffers(1, &id);
        }
    }
    if (count <= 127) {
        for (int i = 0; i < 128 - count; ++i)
            s_buffersToBeDeleted[i] = s_buffersToBeDeleted[i + count];
    }
    s_buffersToBeDeletedPos -= count;
}

}
