#include <stddef.h>

#include "gl_proc.h"

#if defined(_WIN32)
#define GL_APIENTRY __stdcall
#else
#define GL_APIENTRY
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef int GLfixed;
typedef int GLclampx;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;
typedef void GLvoid;

extern "C" {

void glActiveTexture(GLenum texture) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glActiveTexture"));
    real(texture);
}

void glAlphaFunc(GLenum func, GLfloat ref) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLfloat)>(port::gfx::glProc("glAlphaFunc"));
    real(func, ref);
}

void glAttachShader(GLuint program, GLuint shader) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLuint)>(port::gfx::glProc("glAttachShader"));
    real(program, shader);
}

void glBindAttribLocation(GLuint program, GLuint index, const GLchar * name) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLuint, const GLchar *)>(port::gfx::glProc("glBindAttribLocation"));
    real(program, index, name);
}

void glBindBuffer(GLenum target, GLuint buffer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLuint)>(port::gfx::glProc("glBindBuffer"));
    real(target, buffer);
}

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLuint)>(port::gfx::glProc("glBindFramebuffer"));
    real(target, framebuffer);
}

void glBindFramebufferOES(GLenum target, GLuint framebuffer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLuint)>(port::gfx::glProc("glBindFramebufferOES"));
    real(target, framebuffer);
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLuint)>(port::gfx::glProc("glBindRenderbuffer"));
    real(target, renderbuffer);
}

void glBindRenderbufferOES(GLenum target, GLuint renderbuffer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLuint)>(port::gfx::glProc("glBindRenderbufferOES"));
    real(target, renderbuffer);
}

void glBindTexture(GLenum target, GLuint texture) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLuint)>(port::gfx::glProc("glBindTexture"));
    real(target, texture);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum)>(port::gfx::glProc("glBlendFunc"));
    real(sfactor, dfactor);
}

void glBufferData(GLenum target, int size, const void * data, GLenum usage) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLsizeiptr, const void *, GLenum)>(port::gfx::glProc("glBufferData"));
    real(target, static_cast<GLsizeiptr>(size), data, usage);
}

void glBufferSubData(GLenum target, int offset, int size, const void * data) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLintptr, GLsizeiptr, const void *)>(port::gfx::glProc("glBufferSubData"));
    real(target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
}

GLenum glCheckFramebufferStatus(GLenum target) {
    static auto real = reinterpret_cast<GLenum (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glCheckFramebufferStatus"));
    return real(target);
}

GLenum glCheckFramebufferStatusOES(GLenum target) {
    static auto real = reinterpret_cast<GLenum (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glCheckFramebufferStatusOES"));
    return real(target);
}

void glClear(GLbitfield mask) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLbitfield)>(port::gfx::glProc("glClear"));
    real(mask);
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLfloat, GLfloat, GLfloat, GLfloat)>(port::gfx::glProc("glClearColor"));
    real(red, green, blue, alpha);
}

void glClearColorx(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLfixed, GLfixed, GLfixed, GLfixed)>(port::gfx::glProc("glClearColorx"));
    real(red, green, blue, alpha);
}

void glClientActiveTexture(GLenum texture) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glClientActiveTexture"));
    real(texture);
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLfloat, GLfloat, GLfloat, GLfloat)>(port::gfx::glProc("glColor4f"));
    real(red, green, blue, alpha);
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLubyte, GLubyte, GLubyte, GLubyte)>(port::gfx::glProc("glColor4ub"));
    real(red, green, blue, alpha);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLboolean, GLboolean, GLboolean, GLboolean)>(port::gfx::glProc("glColorMask"));
    real(red, green, blue, alpha);
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const void * pointer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLenum, GLsizei, const void *)>(port::gfx::glProc("glColorPointer"));
    real(size, type, stride, pointer);
}

void glCompileShader(GLuint shader) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint)>(port::gfx::glProc("glCompileShader"));
    real(shader);
}

GLuint glCreateProgram() {
    static auto real = reinterpret_cast<GLuint (GL_APIENTRY*)(void)>(port::gfx::glProc("glCreateProgram"));
    return real();
}

GLuint glCreateShader(GLenum type) {
    static auto real = reinterpret_cast<GLuint (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glCreateShader"));
    return real(type);
}

void glCullFace(GLenum mode) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glCullFace"));
    real(mode);
}

void glCurrentPaletteMatrixOES(GLuint matrixpaletteindex) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint)>(port::gfx::glProc("glCurrentPaletteMatrixOES"));
    real(matrixpaletteindex);
}

void glDeleteBuffers(GLsizei n, const GLuint * buffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, const GLuint *)>(port::gfx::glProc("glDeleteBuffers"));
    real(n, buffers);
}

void glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, const GLuint *)>(port::gfx::glProc("glDeleteFramebuffers"));
    real(n, framebuffers);
}

void glDeleteFramebuffersOES(GLsizei n, const GLuint * framebuffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, const GLuint *)>(port::gfx::glProc("glDeleteFramebuffersOES"));
    real(n, framebuffers);
}

void glDeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, const GLuint *)>(port::gfx::glProc("glDeleteRenderbuffers"));
    real(n, renderbuffers);
}

void glDeleteRenderbuffersOES(GLsizei n, const GLuint * renderbuffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, const GLuint *)>(port::gfx::glProc("glDeleteRenderbuffersOES"));
    real(n, renderbuffers);
}

void glDeleteShader(GLuint shader) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint)>(port::gfx::glProc("glDeleteShader"));
    real(shader);
}

void glDeleteTextures(GLsizei n, const GLuint * textures) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, const GLuint *)>(port::gfx::glProc("glDeleteTextures"));
    real(n, textures);
}

void glDepthMask(GLboolean flag) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLboolean)>(port::gfx::glProc("glDepthMask"));
    real(flag);
}

void glDisable(GLenum cap) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glDisable"));
    real(cap);
}

void glDisableClientState(GLenum array) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glDisableClientState"));
    real(array);
}

void glEnable(GLenum cap) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glEnable"));
    real(cap);
}

void glEnableClientState(GLenum array) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glEnableClientState"));
    real(array);
}

void glFogf(GLenum pname, GLfloat param) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLfloat)>(port::gfx::glProc("glFogf"));
    real(pname, param);
}

void glFogfv(GLenum pname, const GLfloat * params) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, const GLfloat *)>(port::gfx::glProc("glFogfv"));
    real(pname, params);
}

void glFogx(GLenum pname, GLfixed param) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLfixed)>(port::gfx::glProc("glFogx"));
    real(pname, param);
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLenum, GLuint)>(port::gfx::glProc("glFramebufferRenderbuffer"));
    real(target, attachment, renderbuffertarget, renderbuffer);
}

void glFramebufferRenderbufferOES(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLenum, GLuint)>(port::gfx::glProc("glFramebufferRenderbufferOES"));
    real(target, attachment, renderbuffertarget, renderbuffer);
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint)>(port::gfx::glProc("glFramebufferTexture2D"));
    real(target, attachment, textarget, texture, level);
}

void glFramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint)>(port::gfx::glProc("glFramebufferTexture2DOES"));
    real(target, attachment, textarget, texture, level);
}

void glFrontFace(GLenum mode) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glFrontFace"));
    real(mode);
}

void glGenBuffers(GLsizei n, GLuint * buffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, GLuint *)>(port::gfx::glProc("glGenBuffers"));
    real(n, buffers);
}

void glGenFramebuffers(GLsizei n, GLuint * framebuffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, GLuint *)>(port::gfx::glProc("glGenFramebuffers"));
    real(n, framebuffers);
}

void glGenFramebuffersOES(GLsizei n, GLuint * framebuffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, GLuint *)>(port::gfx::glProc("glGenFramebuffersOES"));
    real(n, framebuffers);
}

void glGenRenderbuffers(GLsizei n, GLuint * renderbuffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, GLuint *)>(port::gfx::glProc("glGenRenderbuffers"));
    real(n, renderbuffers);
}

void glGenRenderbuffersOES(GLsizei n, GLuint * renderbuffers) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, GLuint *)>(port::gfx::glProc("glGenRenderbuffersOES"));
    real(n, renderbuffers);
}

void glGenTextures(GLsizei n, GLuint * textures) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLsizei, GLuint *)>(port::gfx::glProc("glGenTextures"));
    real(n, textures);
}

GLenum glGetError() {
    static auto real = reinterpret_cast<GLenum (GL_APIENTRY*)(void)>(port::gfx::glProc("glGetError"));
    return real();
}

void glGetFloatv(GLenum pname, GLfloat * data) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLfloat *)>(port::gfx::glProc("glGetFloatv"));
    real(pname, data);
}

void glGetIntegerv(GLenum pname, GLint * data) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLint *)>(port::gfx::glProc("glGetIntegerv"));
    real(pname, data);
}

void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLsizei, GLsizei *, GLchar *)>(port::gfx::glProc("glGetProgramInfoLog"));
    real(program, bufSize, length, infoLog);
}

void glGetProgramiv(GLuint program, GLenum pname, GLint * params) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLenum, GLint *)>(port::gfx::glProc("glGetProgramiv"));
    real(program, pname, params);
}

void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint * params) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLint *)>(port::gfx::glProc("glGetRenderbufferParameteriv"));
    real(target, pname, params);
}

void glGetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint * params) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLint *)>(port::gfx::glProc("glGetRenderbufferParameterivOES"));
    real(target, pname, params);
}

void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLsizei, GLsizei *, GLchar *)>(port::gfx::glProc("glGetShaderInfoLog"));
    real(shader, bufSize, length, infoLog);
}

void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLsizei, GLsizei *, GLchar *)>(port::gfx::glProc("glGetShaderSource"));
    real(shader, bufSize, length, source);
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint * params) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLenum, GLint *)>(port::gfx::glProc("glGetShaderiv"));
    real(shader, pname, params);
}

GLint glGetUniformLocation(GLuint program, const GLchar * name) {
    static auto real = reinterpret_cast<GLint (GL_APIENTRY*)(GLuint, const GLchar *)>(port::gfx::glProc("glGetUniformLocation"));
    return real(program, name);
}

int glIsEnabled(GLenum cap) {
    static auto real = reinterpret_cast<GLboolean (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glIsEnabled"));
    return real(cap);
}

void glLoadIdentity() {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(void)>(port::gfx::glProc("glLoadIdentity"));
    real();
}

void glLoadMatrixf(const GLfloat * m) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(const GLfloat *)>(port::gfx::glProc("glLoadMatrixf"));
    real(m);
}

void glMatrixIndexPointerOES(GLint size, GLenum type, GLsizei stride, const void * pointer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLenum, GLsizei, const void *)>(port::gfx::glProc("glMatrixIndexPointerOES"));
    real(size, type, stride, pointer);
}

void glMatrixMode(GLenum mode) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glMatrixMode"));
    real(mode);
}

void glMultMatrixf(const GLfloat * m) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(const GLfloat *)>(port::gfx::glProc("glMultMatrixf"));
    real(m);
}

void glNormalPointer(GLenum type, GLsizei stride, const void * pointer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLsizei, const void *)>(port::gfx::glProc("glNormalPointer"));
    real(type, stride, pointer);
}

void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat)>(port::gfx::glProc("glOrthof"));
    real(l, r, b, t, n, f);
}

void glPixelStorei(GLenum pname, GLint param) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLint)>(port::gfx::glProc("glPixelStorei"));
    real(pname, param);
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLfloat, GLfloat)>(port::gfx::glProc("glPolygonOffset"));
    real(factor, units);
}

void glPopMatrix() {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(void)>(port::gfx::glProc("glPopMatrix"));
    real();
}

void glPushMatrix() {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(void)>(port::gfx::glProc("glPushMatrix"));
    real();
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLsizei, GLsizei)>(port::gfx::glProc("glRenderbufferStorage"));
    real(target, internalformat, width, height);
}

void glRenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLsizei, GLsizei)>(port::gfx::glProc("glRenderbufferStorageOES"));
    real(target, internalformat, width, height);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLfloat, GLfloat, GLfloat, GLfloat)>(port::gfx::glProc("glRotatef"));
    real(angle, x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLfloat, GLfloat, GLfloat)>(port::gfx::glProc("glScalef"));
    real(x, y, z);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLint, GLsizei, GLsizei)>(port::gfx::glProc("glScissor"));
    real(x, y, width, height);
}

void glShadeModel(GLenum mode) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum)>(port::gfx::glProc("glShadeModel"));
    real(mode);
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLsizei, const GLchar *const*, const GLint *)>(port::gfx::glProc("glShaderSource"));
    real(shader, count, string, length);
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void * pointer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLenum, GLsizei, const void *)>(port::gfx::glProc("glTexCoordPointer"));
    real(size, type, stride, pointer);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLfloat)>(port::gfx::glProc("glTexEnvf"));
    real(target, pname, param);
}

void glTexEnvi(GLenum target, GLenum pname, GLint param) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLint)>(port::gfx::glProc("glTexEnvi"));
    real(target, pname, param);
}

void glTexEnvx(GLenum target, GLenum pname, GLfixed param) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLfixed)>(port::gfx::glProc("glTexEnvx"));
    real(target, pname, param);
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *)>(port::gfx::glProc("glTexImage2D"));
    real(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLint)>(port::gfx::glProc("glTexParameteri"));
    real(target, pname, param);
}

void glTexParameterx(GLenum target, GLenum pname, GLfixed param) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLenum, GLfixed)>(port::gfx::glProc("glTexParameterx"));
    real(target, pname, param);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *)>(port::gfx::glProc("glTexSubImage2D"));
    real(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLfloat, GLfloat, GLfloat)>(port::gfx::glProc("glTranslatef"));
    real(x, y, z);
}

void glUniform1f(GLint location, GLfloat v0) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLfloat)>(port::gfx::glProc("glUniform1f"));
    real(location, v0);
}

void glUniform1fv(GLint location, GLsizei count, const GLfloat * value) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLsizei, const GLfloat *)>(port::gfx::glProc("glUniform1fv"));
    real(location, count, value);
}

void glUniform1i(GLint location, GLint v0) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLint)>(port::gfx::glProc("glUniform1i"));
    real(location, v0);
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLfloat, GLfloat)>(port::gfx::glProc("glUniform2f"));
    real(location, v0, v1);
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat * value) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLsizei, const GLfloat *)>(port::gfx::glProc("glUniform2fv"));
    real(location, count, value);
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat)>(port::gfx::glProc("glUniform3f"));
    real(location, v0, v1, v2);
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat * value) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLsizei, const GLfloat *)>(port::gfx::glProc("glUniform3fv"));
    real(location, count, value);
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat, GLfloat)>(port::gfx::glProc("glUniform4f"));
    real(location, v0, v1, v2, v3);
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat * value) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLsizei, const GLfloat *)>(port::gfx::glProc("glUniform4fv"));
    real(location, count, value);
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLsizei, GLboolean, const GLfloat *)>(port::gfx::glProc("glUniformMatrix4fv"));
    real(location, count, transpose, value);
}

void glValidateProgram(GLuint program) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint)>(port::gfx::glProc("glValidateProgram"));
    real(program);
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *)>(port::gfx::glProc("glVertexAttribPointer"));
    real(index, size, type, normalized, stride, pointer);
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void * pointer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLenum, GLsizei, const void *)>(port::gfx::glProc("glVertexPointer"));
    real(size, type, stride, pointer);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLint, GLsizei, GLsizei)>(port::gfx::glProc("glViewport"));
    real(x, y, width, height);
}

void glWeightPointerOES(GLint size, GLenum type, GLsizei stride, const void * pointer) {
    static auto real = reinterpret_cast<void (GL_APIENTRY*)(GLint, GLenum, GLsizei, const void *)>(port::gfx::glProc("glWeightPointerOES"));
    real(size, type, stride, pointer);
}

}
