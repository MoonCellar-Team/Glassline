#include "runtime/midp/GLES11.h"

extern "C" {

enum {
    GL_FOG = 0x0B60,
    GL_CULL_FACE = 0x0B44,
    GL_DEPTH_TEST = 0x0B71,
    GL_ALPHA_TEST = 0x0BC0,
    GL_ALPHA_TEST_FUNC = 0x0BC1,
    GL_ALPHA_TEST_REF = 0x0BC2,
    GL_BLEND = 0x0BE2,
    GL_VIEWPORT = 0x0BA2,
    GL_MATRIX_MODE = 0x0BA0,
    GL_SMOOTH = 0x1D01,
    GL_VERTEX_ARRAY = 0x8074,
    GL_NORMAL_ARRAY = 0x8075,
    GL_COLOR_ARRAY = 0x8076,
    GL_POLYGON_OFFSET_FILL = 0x8037,
    GL_ACTIVE_TEXTURE = 0x84E0,
    GL_CLIENT_ACTIVE_TEXTURE = 0x84E1,
    GL_FRAMEBUFFER_BINDING = 0x8CA6
};

void glActiveTexture(int texture);
void glAlphaFunc(int func, float ref);
void glBindBuffer(int target, int buffer);
void glBindFramebufferOES(int target, int framebuffer);
void glBindRenderbufferOES(int target, int renderbuffer);
void glBindTexture(int target, int texture);
void glBlendFunc(int sfactor, int dfactor);
void glBufferData(int target, int size, void* data, int usage);
void glBufferSubData(int target, int offset, int size, void* data);
int glCheckFramebufferStatusOES(int target);
void glClientActiveTexture(int texture);
void glColorMask(int r, int g, int b, int a);
void glCompressedTexImage2D(int target, int level, int internalFormat, int width, int height, int border, int imageSize,
                            const void* data);
void glCompressedTexSubImage2D(int target, int level, int xoffset, int yoffset, int width, int height, int format,
                               int imageSize, const void* data);
void glCullFace(int mode);
void glDeleteBuffers(int n, int* buffers);
void glDeleteFramebuffersOES(int n, const unsigned int* framebuffers);
void glDeleteRenderbuffersOES(int n, const unsigned int* renderbuffers);
void glDeleteTextures(int n, const unsigned int* textures);
void glDepthMask(int flag);
void glDisable(int cap);
void glDisableClientState(int array);
void glEnable(int cap);
void glEnableClientState(int array);
void glFramebufferRenderbufferOES(int target, int attachment, int renderbuffertarget, int renderbuffer);
void glFramebufferTexture2DOES(int target, int attachment, int textarget, int texture, int level);
void glFrontFace(int mode);
void glGenBuffers(int n, int* buffers);
void glGenFramebuffersOES(int n, unsigned int* framebuffers);
void glGenRenderbuffersOES(int n, unsigned int* renderbuffers);
void glGenTextures(int n, unsigned int* textures);
int glGetError(void);
void glGetFloatv(int pname, float* params);
void glGetIntegerv(int pname, int* params);
void glGetRenderbufferParameterivOES(int target, int pname, int* params);
unsigned char glIsEnabled(int cap);
void glMatrixMode(int mode);
void glPixelStorei(int pname, int param);
void glPolygonOffset(float factor, float units);
void glRenderbufferStorageOES(int target, int internalformat, int width, int height);
void glShadeModel(int mode);
void glTexImage2D(int target, int level, int internalformat, int width, int height, int border, int format, int type,
                  const void* pixels);
void glTexSubImage2D(int target, int level, int xoffset, int yoffset, int width, int height, int format, int type,
                     const void* pixels);
void glViewport(int x, int y, int width, int height);

}

namespace midp {

static bool enabledExactly(int cap) {
    return ::glIsEnabled(cap) == 1;
}

static bool enabledAny(int cap) {
    return ::glIsEnabled(cap) != 0;
}

static bool* capField11(CacheGLES11* self, int cap) {
    if (cap > 0x8036) {
        if (cap == GL_POLYGON_OFFSET_FILL)
            return &self->m_polygonOffsetFill;
        return 0;
    }
    if (cap > 0x0BE1) {
        if (cap == GL_BLEND)
            return &self->m_blend;
        return 0;
    }
    if (cap >= GL_ALPHA_TEST) {
        if (cap == GL_ALPHA_TEST)
            return &self->m_alphaTest;
        return 0;
    }
    if (cap == GL_CULL_FACE)
        return &self->m_cullFaceEnabled;
    if (cap == GL_FOG)
        return &self->m_fog;
    if (cap == GL_DEPTH_TEST)
        return &self->m_depthTest;
    return 0;
}

static bool* clientField11(CacheGLES11* self, int array) {
    if (array == GL_VERTEX_ARRAY)
        return &self->m_vertexArray;
    if (array == GL_NORMAL_ARRAY)
        return &self->m_normalArray;
    if (array == GL_COLOR_ARRAY)
        return &self->m_colorArray;
    return 0;
}

template <>
const bool CacheGLES11::updateField<int>(int& field, int value) {
    if (field != value) {
        field = value;
        return true;
    }
    return false;
}

template <>
const bool CacheGLES11::updateField<bool>(bool& field, bool value) {
    if (field != value) {
        field = value;
        return true;
    }
    return false;
}

template <>
const bool CacheGLES11::updateField<float>(float& field, float value) {
    if (field != value) {
        field = value;
        return true;
    }
    return false;
}

CacheGLES11::CacheGLES11()
    : m_viewportX(0),
      m_viewportY(0),
      m_viewportW(0),
      m_viewportH(0),
      m_activeTexture(0),
      m_clientActiveTexture(0),
      m_normalArray(false),
      m_colorArray(false),
      m_vertexArray(false),
      m_matrixMode(0),
      m_framebuffer(0),
      m_blend(false),
      m_fog(false),
      m_depthTest(false),
      m_cullFaceEnabled(false),
      m_polygonOffsetFill(false),
      m_alphaTest(false),
      m_blendSrc(1),
      m_blendDst(0),
      m_depthMask(1),
      m_colorR(1),
      m_colorG(1),
      m_colorB(1),
      m_colorA(1),
      m_alphaFunc(0),
      m_alphaRef(0.0f),
      m_shadeModel(GL_SMOOTH),
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
    ::glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture);
    ::glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &m_clientActiveTexture);
    m_normalArray = enabledExactly(GL_NORMAL_ARRAY);
    m_colorArray = enabledExactly(GL_COLOR_ARRAY);
    m_vertexArray = enabledExactly(GL_VERTEX_ARRAY);
    for (int i = 0; i < 8; i++)
        m_unk68[i] = 0;
    ::glGetIntegerv(GL_MATRIX_MODE, &m_matrixMode);
    ::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_framebuffer);
    m_blend = enabledAny(GL_BLEND);
    m_fog = enabledAny(GL_FOG);
    ::glGetIntegerv(GL_ALPHA_TEST_FUNC, &m_alphaFunc);
    ::glGetFloatv(GL_ALPHA_TEST_REF, &m_alphaRef);
}

CacheGLES11::~CacheGLES11() {}

int CacheGLES11::getGLESVersion() {
    return 1;
}

void CacheGLES11::glShadeModel(int mode) {
    if (updateField(m_shadeModel, mode))
        ::glShadeModel(mode);
}

void CacheGLES11::glAlphaFunc(int func, float ref) {
    bool funcChanged = updateField(m_alphaFunc, func);
    bool refChanged = updateField(m_alphaRef, ref);
    if (funcChanged || refChanged)
        ::glAlphaFunc(func, ref);
}

void CacheGLES11::glMatrixMode(int mode) {
    if (updateField(m_matrixMode, mode))
        ::glMatrixMode(mode);
}

void CacheGLES11::glPolygonOffset(float factor, float units) {
    bool factorChanged = updateField(m_polygonFactor, factor);
    bool unitsChanged = updateField(m_polygonUnits, units);
    if (factorChanged || unitsChanged)
        ::glPolygonOffset(factor, units);
}

void CacheGLES11::glFrontFace(int mode) {
    if (updateField(m_frontFace, mode))
        ::glFrontFace(mode);
}

void CacheGLES11::glCullFace(int mode) {
    if (updateField(m_cullFace, mode))
        ::glCullFace(mode);
}

void CacheGLES11::glColorMask(int r, int g, int b, int a) {
    bool cr = updateField(m_colorR, r);
    bool cg = updateField(m_colorG, g);
    bool cb = updateField(m_colorB, b);
    bool ca = updateField(m_colorA, a);
    if (cr || cg || cb || ca)
        ::glColorMask(r & 0xff, g & 0xff, b & 0xff, a & 0xff);
}

void CacheGLES11::glDepthMask(int flag) {
    if (updateField(m_depthMask, flag))
        ::glDepthMask(flag & 0xff);
}

void CacheGLES11::glBlendFunc(int sfactor, int dfactor) {
    bool srcChanged = updateField(m_blendSrc, sfactor);
    bool dstChanged = updateField(m_blendDst, dfactor);
    if (srcChanged || dstChanged)
        ::glBlendFunc(sfactor, dfactor);
}

void CacheGLES11::glRenderbufferStorage(int target, int format, int width, int height) {
    ::glRenderbufferStorageOES(target, format, width, height);
}

void CacheGLES11::glGetRenderbufferParameteriv(int target, int pname, int* params) {
    ::glGetRenderbufferParameterivOES(target, pname, params);
}

void CacheGLES11::glGenRenderbuffers(int n, unsigned int* renderbuffers) {
    ::glGenRenderbuffersOES(n, renderbuffers);
}

void CacheGLES11::glGenFramebuffers(int n, unsigned int* framebuffers) {
    ::glGenFramebuffersOES(n, framebuffers);
}

void CacheGLES11::glFramebufferTexture2D(int target, int attachment, int texTarget, int texture, int level) {
    ::glFramebufferTexture2DOES(target, attachment, texTarget, texture, level);
}

void CacheGLES11::glFramebufferRenderbuffer(int target, int attachment, int rbTarget, int renderbuffer) {
    ::glFramebufferRenderbufferOES(target, attachment, rbTarget, renderbuffer);
}

void CacheGLES11::glDeleteRenderbuffers(int n, const unsigned int* renderbuffers) {
    ::glDeleteRenderbuffersOES(n, renderbuffers);
}

void CacheGLES11::glDeleteFramebuffers(int n, const unsigned int* framebuffers) {
    ::glDeleteFramebuffersOES(n, framebuffers);
}

int CacheGLES11::glCheckFramebufferStatus(int target) {
    return ::glCheckFramebufferStatusOES(target);
}

void CacheGLES11::glBindRenderbuffer(int target, int renderbuffer) {
    ::glBindRenderbufferOES(target, renderbuffer);
}

void CacheGLES11::glBindFramebuffer(int target, int framebuffer) {
    if (updateField(m_framebuffer, framebuffer))
        ::glBindFramebufferOES(target, framebuffer);
}

void CacheGLES11::glBufferSubData(int target, int offset, int size, void* data) {
    ::glBufferSubData(target, offset, size, data);
}

void CacheGLES11::glBufferData(int target, int size, void* data, int usage) {
    ::glBufferData(target, size, data, usage);
}

void CacheGLES11::glBindBuffer(int target, int buffer) {
    ::glBindBuffer(target, buffer);
}

void CacheGLES11::glDeleteBuffers(int n, int* buffers) {
    ::glDeleteBuffers(n, buffers);
}

void CacheGLES11::glGenBuffers(int n, int* buffers) {
    ::glGenBuffers(n, buffers);
}

void CacheGLES11::glViewport(int x, int y, int width, int height) {
    bool cx = updateField(m_viewportX, x);
    bool cy = updateField(m_viewportY, y);
    bool cw = updateField(m_viewportW, width);
    bool ch = updateField(m_viewportH, height);
    if (cx || cy || cw || ch)
        ::glViewport(x, y, width, height);
}

void CacheGLES11::glCompressedTexImage2D(int target, int level, int internalFormat, int width, int height, int border,
                                        int size, const void* data) {
    ::glCompressedTexImage2D(target, level, internalFormat, width, height, border, size, data);
}

void CacheGLES11::glCompressedTexSubImage2D(int target, int level, int x, int y, int width, int height, int format,
                                           int size, const void* data) {
    ::glCompressedTexSubImage2D(target, level, x, y, width, height, format, size, data);
}

void CacheGLES11::glTexImage2D(int target, int level, int internalFormat, int width, int height, int border, int format,
                              int type, const void* pixels) {
    ::glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
}

void CacheGLES11::glTexSubImage2D(int target, int level, int x, int y, int width, int height, int format, int type,
                                 const void* pixels) {
    ::glTexSubImage2D(target, level, x, y, width, height, format, type, pixels);
}

int CacheGLES11::glGetError() {
    return ::glGetError();
}

void CacheGLES11::glGetIntegerv(int pname, int* params) {
    if (pname == GL_FRAMEBUFFER_BINDING) {
        *params = m_framebuffer;
        return;
    }
    ::glGetIntegerv(pname, params);
}

void CacheGLES11::glEnableClientState(int array) {
    bool* field = clientField11(this, array);
    if (field == 0 || updateField(*field, true))
        ::glEnableClientState(array);
}

void CacheGLES11::glEnable(int cap) {
    bool* field = capField11(this, cap);
    if (field == 0 || updateField(*field, true))
        ::glEnable(cap);
}

void CacheGLES11::glDisableClientState(int array) {
    bool* field = clientField11(this, array);
    if (field == 0 || updateField(*field, false))
        ::glDisableClientState(array);
}

void CacheGLES11::glDisable(int cap) {
    bool* field = capField11(this, cap);
    if (field == 0 || updateField(*field, false))
        ::glDisable(cap);
}

void CacheGLES11::glDeleteTextures(int n, const unsigned int* textures) {
    ::glDeleteTextures(n, textures);
}

void CacheGLES11::glGenTextures(int n, unsigned int* textures) {
    ::glGenTextures(n, textures);
}

void CacheGLES11::glClientActiveTexture(int texture) {
    if (updateField(m_clientActiveTexture, texture))
        ::glClientActiveTexture(texture);
}

void CacheGLES11::glBindTexture(int target, int texture) {
    ::glBindTexture(target, texture);
}

void CacheGLES11::glActiveTexture(int texture) {
    if (updateField(m_activeTexture, texture))
        ::glActiveTexture(texture);
}

void CacheGLES11::glPixelStorei(int pname, int param) {
    ::glPixelStorei(pname, param);
}

}
