#pragma once

#include "runtime/midp/GLES.h"

namespace midp {

class GLES11 : public GLES {
public:
    GLES11();
    virtual ~GLES11();
    virtual void glMatrixMode(int mode) = 0;
    virtual void glAlphaFunc(int func, float ref) = 0;
    virtual void glShadeModel(int mode) = 0;
};

class CacheGLES11 : public GLES11 {
public:
    CacheGLES11();
    virtual ~CacheGLES11();

    virtual void glMatrixMode(int mode);
    virtual void glAlphaFunc(int func, float ref);
    virtual void glShadeModel(int mode);

    virtual void glPixelStorei(int pname, int param);
    virtual void glActiveTexture(int texture);
    virtual void glBindTexture(int target, int texture);
    virtual void glClientActiveTexture(int texture);
    virtual void glGenTextures(int n, unsigned int* textures);
    virtual void glDeleteTextures(int n, const unsigned int* textures);
    virtual void glDisable(int cap);
    virtual void glDisableClientState(int array);
    virtual void glEnable(int cap);
    virtual void glEnableClientState(int array);
    virtual void glGetIntegerv(int pname, int* params);
    virtual int glGetError();
    virtual void glTexSubImage2D(int target, int level, int x, int y, int width, int height, int format, int type,
                                 const void* pixels);
    virtual void glTexImage2D(int target, int level, int internalFormat, int width, int height, int border, int format,
                              int type, const void* pixels);
    virtual void glCompressedTexSubImage2D(int target, int level, int x, int y, int width, int height, int format,
                                           int size, const void* data);
    virtual void glCompressedTexImage2D(int target, int level, int internalFormat, int width, int height, int border,
                                        int size, const void* data);
    virtual void glViewport(int x, int y, int width, int height);
    virtual void glGenBuffers(int n, int* buffers);
    virtual void glDeleteBuffers(int n, int* buffers);
    virtual void glBindBuffer(int target, int buffer);
    virtual void glBufferData(int target, int size, void* data, int usage);
    virtual void glBufferSubData(int target, int offset, int size, void* data);
    virtual void glBindFramebuffer(int target, int framebuffer);
    virtual void glBindRenderbuffer(int target, int renderbuffer);
    virtual int glCheckFramebufferStatus(int target);
    virtual void glDeleteFramebuffers(int n, const unsigned int* framebuffers);
    virtual void glDeleteRenderbuffers(int n, const unsigned int* renderbuffers);
    virtual void glFramebufferRenderbuffer(int target, int attachment, int rbTarget, int renderbuffer);
    virtual void glFramebufferTexture2D(int target, int attachment, int texTarget, int texture, int level);
    virtual void glGenFramebuffers(int n, unsigned int* framebuffers);
    virtual void glGenRenderbuffers(int n, unsigned int* renderbuffers);
    virtual void glGetRenderbufferParameteriv(int target, int pname, int* params);
    virtual void glRenderbufferStorage(int target, int format, int width, int height);
    virtual void glBlendFunc(int sfactor, int dfactor);
    virtual void glDepthMask(int flag);
    virtual void glColorMask(int r, int g, int b, int a);
    virtual void glCullFace(int mode);
    virtual void glFrontFace(int mode);
    virtual void glPolygonOffset(float factor, float units);
    virtual int getGLESVersion();

    template <class T>
    const bool updateField(T& field, T value);

    int m_viewportX;
    int m_viewportY;
    int m_viewportW;
    int m_viewportH;
    int m_activeTexture;
    int m_clientActiveTexture;
    bool m_normalArray;
    bool m_colorArray;
    bool m_vertexArray;
    int m_matrixMode;
    int m_framebuffer;
    bool m_blend;
    bool m_fog;
    bool m_depthTest;
    bool m_cullFaceEnabled;
    bool m_polygonOffsetFill;
    bool m_alphaTest;
    int m_blendSrc;
    int m_blendDst;
    int m_depthMask;
    int m_colorR;
    int m_colorG;
    int m_colorB;
    int m_colorA;
    int m_alphaFunc;
    float m_alphaRef;
    int m_shadeModel;
    int m_cullFace;
    int m_frontFace;
    float m_polygonFactor;
    float m_polygonUnits;
    int m_unk68[8];
};

}
