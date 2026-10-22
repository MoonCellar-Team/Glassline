#include "runtime/midp/GLES20.h"

#include "sdk/string.h"

extern "C" {

enum {
    GL_CULL_FACE = 0x0B44,
    GL_DEPTH_TEST = 0x0B71,
    GL_BLEND = 0x0BE2,
    GL_VIEWPORT = 0x0BA2,
    GL_POLYGON_OFFSET_FILL = 0x8037,
    GL_FRAMEBUFFER_BINDING = 0x8CA6
};

void glActiveTexture(int texture);
void glBindBuffer(int target, int buffer);
void glBindFramebuffer(int target, int framebuffer);
void glBindRenderbuffer(int target, int renderbuffer);
void glBindTexture(int target, int texture);
void glBlendFunc(int sfactor, int dfactor);
void glBufferData(int target, int size, void* data, int usage);
void glBufferSubData(int target, int offset, int size, void* data);
int glCheckFramebufferStatus(int target);
void glColorMask(int r, int g, int b, int a);
void glCompressedTexImage2D(int target, int level, int internalFormat, int width, int height, int border, int imageSize,
                            const void* data);
void glCompressedTexSubImage2D(int target, int level, int xoffset, int yoffset, int width, int height, int format,
                               int imageSize, const void* data);
void glCullFace(int mode);
void glDeleteBuffers(int n, int* buffers);
void glDeleteFramebuffers(int n, const unsigned int* framebuffers);
void glDeleteRenderbuffers(int n, const unsigned int* renderbuffers);
void glDeleteTextures(int n, const unsigned int* textures);
void glDepthMask(int flag);
void glDisable(int cap);
void glDisableVertexAttribArray(unsigned int index);
void glEnable(int cap);
void glEnableVertexAttribArray(unsigned int index);
void glFramebufferRenderbuffer(int target, int attachment, int renderbuffertarget, int renderbuffer);
void glFramebufferTexture2D(int target, int attachment, int textarget, int texture, int level);
void glFrontFace(int mode);
void glGenBuffers(int n, int* buffers);
void glGenFramebuffers(int n, unsigned int* framebuffers);
void glGenRenderbuffers(int n, unsigned int* renderbuffers);
void glGenTextures(int n, unsigned int* textures);
int glGetError(void);
void glGetIntegerv(int pname, int* params);
void glGetRenderbufferParameteriv(int target, int pname, int* params);
void glPixelStorei(int pname, int param);
void glPolygonOffset(float factor, float units);
void glRenderbufferStorage(int target, int internalformat, int width, int height);
void glTexImage2D(int target, int level, int internalformat, int width, int height, int border, int format, int type,
                  const void* pixels);
void glTexSubImage2D(int target, int level, int xoffset, int yoffset, int width, int height, int format, int type,
                     const void* pixels);
void glUniform1f(int location, float x);
void glUniform1fv(int location, int count, const float* v);
void glUniform1i(int location, int x);
void glUniform2f(int location, float x, float y);
void glUniform2fv(int location, int count, const float* v);
void glUniform3f(int location, float x, float y, float z);
void glUniform3fv(int location, int count, const float* v);
void glUniform4f(int location, float x, float y, float z, float w);
void glUniform4fv(int location, int count, const float* v);
void glUniformMatrix4fv(int location, int count, int transpose, const float* value);
void glUseProgram(int program);
void glViewport(int x, int y, int width, int height);

}

namespace midp {

static bool* capField20(CacheGLES20* self, int cap) {
    if (cap > 0x8036) {
        if (cap == GL_POLYGON_OFFSET_FILL)
            return &self->m_polygonOffsetFill;
        return 0;
    }
    if (cap == GL_CULL_FACE)
        return &self->m_cullFaceEnabled;
    if (cap == GL_DEPTH_TEST)
        return &self->m_depthTest;
    if (cap == GL_BLEND)
        return &self->m_blend;
    return 0;
}

template <>
const bool CacheGLES20::updateField<int>(int& field, int value) {
    if (field != value) {
        field = value;
        return true;
    }
    return false;
}

template <>
const bool CacheGLES20::updateField<bool>(bool& field, bool value) {
    if (field != value) {
        field = value;
        return true;
    }
    return false;
}

template <>
const bool CacheGLES20::updateField<float>(float& field, float value) {
    if (field != value) {
        field = value;
        return true;
    }
    return false;
}

CacheGLES20::CacheGLES20()
    : m_blend(false),
      m_depthTest(false),
      m_cullFaceEnabled(false),
      m_polygonOffsetFill(false),
      m_activeTexture(0),
      m_viewportX(0),
      m_viewportY(0),
      m_viewportW(0),
      m_viewportH(0),
      m_unk2C(0),
      m_framebuffer(0),
      m_program(-1),
      m_blendSrc(1),
      m_blendDst(0),
      m_depthMask(1),
      m_colorR(1),
      m_colorG(1),
      m_colorB(1),
      m_colorA(1),
      m_cullFace(0),
      m_frontFace(0),
      m_polygonFactor(0.0f),
      m_polygonUnits(0.0f) {
    int viewport[4];
    ::glGetIntegerv(GL_VIEWPORT, viewport);
    m_viewportX = viewport[0];
    m_viewportY = viewport[1];
    m_viewportW = viewport[2];
    m_viewportH = viewport[3];
    memset(m_vertexAttrib, 0, sizeof(m_vertexAttrib));
    ::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_framebuffer);
    for (int i = 0; i < 8; i++)
        m_unk54[i] = 0;
}

CacheGLES20::~CacheGLES20() {}

int CacheGLES20::getGLESVersion() {
    return 2;
}

void CacheGLES20::glDisableClientState(int) {}

void CacheGLES20::glClientActiveTexture(int) {}

void CacheGLES20::glEnableClientState(int) {}

void CacheGLES20::glUseProgram(int program) {
    if (updateField(m_program, program))
        ::glUseProgram(program);
}

void CacheGLES20::glUniformMatrix4fv(int location, int count, int transpose, const float* m) {
    ::glUniformMatrix4fv(location, count, transpose & 0xff, m);
}

void CacheGLES20::glUniform4fv(int location, int count, const float* v) {
    ::glUniform4fv(location, count, v);
}

void CacheGLES20::glUniform3fv(int location, int count, const float* v) {
    ::glUniform3fv(location, count, v);
}

void CacheGLES20::glUniform2fv(int location, int count, const float* v) {
    ::glUniform2fv(location, count, v);
}

void CacheGLES20::glUniform1fv(int location, int count, const float* v) {
    ::glUniform1fv(location, count, v);
}

void CacheGLES20::glUniform4f(int location, float x, float y, float z, float w) {
    ::glUniform4f(location, x, y, z, w);
}

void CacheGLES20::glUniform3f(int location, float x, float y, float z) {
    ::glUniform3f(location, x, y, z);
}

void CacheGLES20::glUniform2f(int location, float x, float y) {
    ::glUniform2f(location, x, y);
}

void CacheGLES20::glUniform1f(int location, float x) {
    ::glUniform1f(location, x);
}

void CacheGLES20::glUniform1i(int location, int x) {
    ::glUniform1i(location, x);
}

void CacheGLES20::glDisableVertexAttribArray(unsigned int index) {
    if (updateField(m_vertexAttrib[index], false))
        ::glDisableVertexAttribArray(index);
}

void CacheGLES20::glEnableVertexAttribArray(unsigned int index) {
    if (updateField(m_vertexAttrib[index], true))
        ::glEnableVertexAttribArray(index);
}

void CacheGLES20::glPolygonOffset(float factor, float units) {
    bool factorChanged = updateField(m_polygonFactor, factor);
    bool unitsChanged = updateField(m_polygonUnits, units);
    if (factorChanged || unitsChanged)
        ::glPolygonOffset(factor, units);
}

void CacheGLES20::glFrontFace(int mode) {
    if (updateField(m_frontFace, mode))
        ::glFrontFace(mode);
}

void CacheGLES20::glCullFace(int mode) {
    if (updateField(m_cullFace, mode))
        ::glCullFace(mode);
}

void CacheGLES20::glColorMask(int r, int g, int b, int a) {
    bool cr = updateField(m_colorR, r);
    bool cg = updateField(m_colorG, g);
    bool cb = updateField(m_colorB, b);
    bool ca = updateField(m_colorA, a);
    if (cr || cg || cb || ca)
        ::glColorMask(r & 0xff, g & 0xff, b & 0xff, a & 0xff);
}

void CacheGLES20::glDepthMask(int flag) {
    if (updateField(m_depthMask, flag))
        ::glDepthMask(flag & 0xff);
}

void CacheGLES20::glBlendFunc(int sfactor, int dfactor) {
    bool srcChanged = updateField(m_blendSrc, sfactor);
    bool dstChanged = updateField(m_blendDst, dfactor);
    if (srcChanged || dstChanged)
        ::glBlendFunc(sfactor, dfactor);
}

void CacheGLES20::glRenderbufferStorage(int target, int format, int width, int height) {
    ::glRenderbufferStorage(target, format, width, height);
}

void CacheGLES20::glGetRenderbufferParameteriv(int target, int pname, int* params) {
    ::glGetRenderbufferParameteriv(target, pname, params);
}

void CacheGLES20::glGenRenderbuffers(int n, unsigned int* renderbuffers) {
    ::glGenRenderbuffers(n, renderbuffers);
}

void CacheGLES20::glGenFramebuffers(int n, unsigned int* framebuffers) {
    ::glGenFramebuffers(n, framebuffers);
}

void CacheGLES20::glFramebufferTexture2D(int target, int attachment, int texTarget, int texture, int level) {
    ::glFramebufferTexture2D(target, attachment, texTarget, texture, level);
}

void CacheGLES20::glFramebufferRenderbuffer(int target, int attachment, int rbTarget, int renderbuffer) {
    ::glFramebufferRenderbuffer(target, attachment, rbTarget, renderbuffer);
}

void CacheGLES20::glDeleteRenderbuffers(int n, const unsigned int* renderbuffers) {
    ::glDeleteRenderbuffers(n, renderbuffers);
}

void CacheGLES20::glDeleteFramebuffers(int n, const unsigned int* framebuffers) {
    ::glDeleteFramebuffers(n, framebuffers);
}

int CacheGLES20::glCheckFramebufferStatus(int target) {
    return ::glCheckFramebufferStatus(target);
}

void CacheGLES20::glBindRenderbuffer(int target, int renderbuffer) {
    ::glBindRenderbuffer(target, renderbuffer);
}

void CacheGLES20::glBindFramebuffer(int target, int framebuffer) {
    if (updateField(m_framebuffer, framebuffer))
        ::glBindFramebuffer(target, framebuffer);
}

void CacheGLES20::glBufferSubData(int target, int offset, int size, void* data) {
    ::glBufferSubData(target, offset, size, data);
}

void CacheGLES20::glBufferData(int target, int size, void* data, int usage) {
    ::glBufferData(target, size, data, usage);
}

void CacheGLES20::glBindBuffer(int target, int buffer) {
    ::glBindBuffer(target, buffer);
}

void CacheGLES20::glDeleteBuffers(int n, int* buffers) {
    ::glDeleteBuffers(n, buffers);
}

void CacheGLES20::glGenBuffers(int n, int* buffers) {
    ::glGenBuffers(n, buffers);
}

void CacheGLES20::glViewport(int x, int y, int width, int height) {
    bool cx = updateField(m_viewportX, x);
    bool cy = updateField(m_viewportY, y);
    bool cw = updateField(m_viewportW, width);
    bool ch = updateField(m_viewportH, height);
    if (cx || cy || cw || ch)
        ::glViewport(x, y, width, height);
}

void CacheGLES20::glCompressedTexImage2D(int target, int level, int internalFormat, int width, int height, int border,
                                        int size, const void* data) {
    ::glCompressedTexImage2D(target, level, internalFormat, width, height, border, size, data);
}

void CacheGLES20::glCompressedTexSubImage2D(int target, int level, int x, int y, int width, int height, int format,
                                           int size, const void* data) {
    ::glCompressedTexSubImage2D(target, level, x, y, width, height, format, size, data);
}

void CacheGLES20::glTexImage2D(int target, int level, int internalFormat, int width, int height, int border, int format,
                              int type, const void* pixels) {
    ::glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
}

void CacheGLES20::glTexSubImage2D(int target, int level, int x, int y, int width, int height, int format, int type,
                                 const void* pixels) {
    ::glTexSubImage2D(target, level, x, y, width, height, format, type, pixels);
}

int CacheGLES20::glGetError() {
    return ::glGetError();
}

void CacheGLES20::glGetIntegerv(int pname, int* params) {
    ::glGetIntegerv(pname, params);
}

void CacheGLES20::glEnable(int cap) {
    bool* field = capField20(this, cap);
    if (field == 0 || updateField(*field, true))
        ::glEnable(cap);
}

void CacheGLES20::glDisable(int cap) {
    bool* field = capField20(this, cap);
    if (field == 0 || updateField(*field, false))
        ::glDisable(cap);
}

void CacheGLES20::glDeleteTextures(int n, const unsigned int* textures) {
    ::glDeleteTextures(n, textures);
}

void CacheGLES20::glGenTextures(int n, unsigned int* textures) {
    ::glGenTextures(n, textures);
}

void CacheGLES20::glBindTexture(int target, int texture) {
    ::glBindTexture(target, texture);
}

void CacheGLES20::glActiveTexture(int texture) {
    if (updateField(m_activeTexture, texture))
        ::glActiveTexture(texture);
}

void CacheGLES20::glPixelStorei(int pname, int param) {
    ::glPixelStorei(pname, param);
}

}
