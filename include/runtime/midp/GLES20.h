#pragma once

#include "runtime/midp/GLES.h"

namespace midp {

class GLES20 : public GLES {
public:
    GLES20();
    virtual ~GLES20();

    virtual void glEnableVertexAttribArray(unsigned int index) = 0;
    virtual void glDisableVertexAttribArray(unsigned int index) = 0;
    virtual void glUniform1i(int location, int x) = 0;
    virtual void glUniform1f(int location, float x) = 0;
    virtual void glUniform2f(int location, float x, float y) = 0;
    virtual void glUniform3f(int location, float x, float y, float z) = 0;
    virtual void glUniform4f(int location, float x, float y, float z, float w) = 0;
    virtual void glUniform1fv(int location, int count, const float* v) = 0;
    virtual void glUniform2fv(int location, int count, const float* v) = 0;
    virtual void glUniform3fv(int location, int count, const float* v) = 0;
    virtual void glUniform4fv(int location, int count, const float* v) = 0;
    virtual void glUniformMatrix4fv(int location, int count, int transpose, const float* m) = 0;
    virtual void glUseProgram(int program) = 0;
};

class CacheGLES20 : public GLES20 {
public:
    CacheGLES20();
    virtual ~CacheGLES20();

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
    virtual void glEnableVertexAttribArray(unsigned int index);
    virtual void glDisableVertexAttribArray(unsigned int index);
    virtual void glUniform1i(int location, int x);
    virtual void glUniform1f(int location, float x);
    virtual void glUniform2f(int location, float x, float y);
    virtual void glUniform3f(int location, float x, float y, float z);
    virtual void glUniform4f(int location, float x, float y, float z, float w);
    virtual void glUniform1fv(int location, int count, const float* v);
    virtual void glUniform2fv(int location, int count, const float* v);
    virtual void glUniform3fv(int location, int count, const float* v);
    virtual void glUniform4fv(int location, int count, const float* v);
    virtual void glUniformMatrix4fv(int location, int count, int transpose, const float* m);
    virtual void glUseProgram(int program);

    template <class T>
    const bool updateField(T& field, T value);

    bool m_vertexAttrib[16];
    bool m_blend;
    bool m_depthTest;
    bool m_cullFaceEnabled;
    bool m_polygonOffsetFill;
    int m_activeTexture;
    int m_viewportX;
    int m_viewportY;
    int m_viewportW;
    int m_viewportH;
    int m_unk2C;
    int m_framebuffer;
    int m_program;
    int m_blendSrc;
    int m_blendDst;
    int m_depthMask;
    int m_colorR;
    int m_colorG;
    int m_colorB;
    int m_colorA;
    int m_unk54[8];
    int m_cullFace;
    int m_frontFace;
    float m_polygonFactor;
    float m_polygonUnits;
};

}
