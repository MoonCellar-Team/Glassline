#pragma once

namespace midp {

class CacheGLES11;
class CacheGLES20;

class GLES {
public:
    GLES();
    virtual ~GLES();

    static CacheGLES11* InstanceCachedGLES11();
    static CacheGLES20* InstanceCachedGLES20();

    virtual void glPixelStorei(int pname, int param) = 0;
    virtual void glActiveTexture(int texture) = 0;
    virtual void glBindTexture(int target, int texture) = 0;
    virtual void glClientActiveTexture(int texture) = 0;
    virtual void glGenTextures(int n, unsigned int* textures) = 0;
    virtual void glDeleteTextures(int n, const unsigned int* textures) = 0;
    virtual void glDisable(int cap) = 0;
    virtual void glDisableClientState(int array) = 0;
    virtual void glEnable(int cap) = 0;
    virtual void glEnableClientState(int array) = 0;
    virtual void glGetIntegerv(int pname, int* params) = 0;
    virtual int glGetError() = 0;
    virtual void glTexSubImage2D(int target, int level, int x, int y, int width, int height, int format,
                                 int type, const void* pixels) = 0;
    virtual void glTexImage2D(int target, int level, int internalFormat, int width, int height, int border,
                              int format, int type, const void* pixels) = 0;
    virtual void glCompressedTexSubImage2D(int target, int level, int x, int y, int width, int height,
                                           int format, int size, const void* data) = 0;
    virtual void glCompressedTexImage2D(int target, int level, int internalFormat, int width, int height,
                                        int border, int size, const void* data) = 0;
    virtual void glViewport(int x, int y, int width, int height) = 0;
    virtual void glGenBuffers(int n, int* buffers) = 0;
    virtual void glDeleteBuffers(int n, int* buffers) = 0;
    virtual void glBindBuffer(int target, int buffer) = 0;
    virtual void glBufferData(int target, int size, void* data, int usage) = 0;
    virtual void glBufferSubData(int target, int offset, int size, void* data) = 0;
    virtual void glBindFramebuffer(int target, int framebuffer) = 0;
    virtual void glBindRenderbuffer(int target, int renderbuffer) = 0;
    virtual int glCheckFramebufferStatus(int target) = 0;
    virtual void glDeleteFramebuffers(int n, const unsigned int* framebuffers) = 0;
    virtual void glDeleteRenderbuffers(int n, const unsigned int* renderbuffers) = 0;
    virtual void glFramebufferRenderbuffer(int target, int attachment, int rbTarget, int renderbuffer) = 0;
    virtual void glFramebufferTexture2D(int target, int attachment, int texTarget, int texture,
                                        int level) = 0;
    virtual void glGenFramebuffers(int n, unsigned int* framebuffers) = 0;
    virtual void glGenRenderbuffers(int n, unsigned int* renderbuffers) = 0;
    virtual void glGetRenderbufferParameteriv(int target, int pname, int* params) = 0;
    virtual void glRenderbufferStorage(int target, int format, int width, int height) = 0;
    virtual void glBlendFunc(int sfactor, int dfactor) = 0;
    virtual void glDepthMask(int flag) = 0;
    virtual void glColorMask(int r, int g, int b, int a) = 0;
    virtual void glCullFace(int mode) = 0;
    virtual void glFrontFace(int mode) = 0;
    virtual void glPolygonOffset(float factor, float units) = 0;
    virtual int getGLESVersion() = 0;
};

}
