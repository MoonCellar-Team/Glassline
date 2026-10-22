#pragma once

namespace midp {

class GLES;
class GLES11;

class OpenGLESManager {
public:
    static void setGLES(GLES* gl);
    static GLES11* getGLES();
    static void removeTexture(int texture);
    static void removeBuffer(int buffer);
    static void flushDeletedTextures();

    static GLES* m_gl;
    static int s_texturesToBeDeletedPos;
    static int s_texturesToBeDeleted[128];
    static int s_buffersToBeDeletedPos;
    static int s_buffersToBeDeleted[128];
};

}
