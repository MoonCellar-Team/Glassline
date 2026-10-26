#include "attribs.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <vector>

#include "gl_proc.h"

#if defined(_WIN32)
#define GL_APIENTRY __stdcall
#else
#define GL_APIENTRY
#endif

typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef char GLchar;
typedef unsigned char GLboolean;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

namespace {

template <class F>
F proc(const char* name) {
    return reinterpret_cast<F>(port::gfx::glProc(name));
}

unsigned s_wanted;
unsigned s_on;
GLuint s_program;
std::map<GLuint, unsigned> s_inputs;
#ifdef __EMSCRIPTEN__
GLuint s_arrayBuffer, s_elementBuffer;
unsigned s_client;
std::map<GLuint, std::vector<unsigned char>> s_elements;
#endif

void setArray(unsigned index, bool on) {
    static auto enable = proc<void (GL_APIENTRY*)(GLuint)>("glEnableVertexAttribArray");
    static auto disable = proc<void (GL_APIENTRY*)(GLuint)>("glDisableVertexAttribArray");
    if (on)
        enable(index);
    else
        disable(index);
}

unsigned inputsOf(GLuint program) {
    if (!program) return ~0u;
    auto it = s_inputs.find(program);
    if (it != s_inputs.end()) return it->second;
    static auto getProgram = proc<void (GL_APIENTRY*)(GLuint, GLenum, GLint*)>("glGetProgramiv");
    static auto active = proc<void (GL_APIENTRY*)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*)>("glGetActiveAttrib");
    static auto location = proc<GLint (GL_APIENTRY*)(GLuint, const GLchar*)>("glGetAttribLocation");
    GLint linked = 0, count = 0;
    getProgram(program, 0x8B82, &linked);
    if (!linked) return ~0u;
    getProgram(program, 0x8B89, &count);
    unsigned mask = 0;
    for (GLint i = 0; i < count; i++) {
        GLchar name[128];
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;
        active(program, GLuint(i), GLsizei(sizeof name), &length, &size, &type, name);
        GLint loc = location(program, name);
        if (loc < 0) continue;
        int columns = type == 0x8B5A ? 2 : type == 0x8B5B ? 3 : type == 0x8B5C ? 4 : 1;
        for (int k = 0; k < columns * (size > 0 ? size : 1); k++)
            if (loc + k < 32)
                mask |= 1u << (loc + k);
            else
                mask = ~0u;
    }
    s_inputs[program] = mask;
    return mask;
}

void beforeDraw() {
    unsigned want = s_wanted & inputsOf(s_program);
    for (unsigned diff = want ^ s_on; diff; diff &= diff - 1) {
        unsigned index = unsigned(__builtin_ctz(diff));
        setArray(index, (want >> index) & 1);
    }
    s_on = want;
}

}

namespace port {
namespace gfx {

void releaseGameArrays() {
    for (unsigned on = s_on; on; on &= on - 1) setArray(unsigned(__builtin_ctz(on)), false);
    s_on = 0;
}

}
}

extern "C" {

void glEnableVertexAttribArray(GLuint index) {
    MEDECOMP_GL_GUARD();
    if (index >= 32) return setArray(index, true);
    s_wanted |= 1u << index;
}

void glDisableVertexAttribArray(GLuint index) {
    MEDECOMP_GL_GUARD();
    if (index >= 32) return setArray(index, false);
    s_wanted &= ~(1u << index);
    if ((s_on >> index) & 1) {
        setArray(index, false);
        s_on &= ~(1u << index);
    }
}

void glUseProgram(GLuint program) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLuint)>("glUseProgram");
    s_program = program;
    real(program);
}

void glLinkProgram(GLuint program) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLuint)>("glLinkProgram");
    s_inputs.erase(program);
    real(program);
}

void glDeleteProgram(GLuint program) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLuint)>("glDeleteProgram");
    s_inputs.erase(program);
    real(program);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLenum, GLint, GLsizei)>("glDrawArrays");
    beforeDraw();
    real(mode, first, count);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLenum, GLsizei, GLenum, const void*)>("glDrawElements");
    beforeDraw();
#ifdef __EMSCRIPTEN__
    auto copy = s_elementBuffer && (s_on & s_client) ? s_elements.find(s_elementBuffer) : s_elements.end();
    if (copy != s_elements.end()) {
        static auto bind = proc<void (GL_APIENTRY*)(GLenum, GLuint)>("glBindBuffer");
        bind(0x8893, 0);
        real(mode, count, type, copy->second.data() + reinterpret_cast<size_t>(indices));
        bind(0x8893, s_elementBuffer);
        return;
    }
#endif
    real(mode, count, type, indices);
}

void glBindBuffer(GLenum target, GLuint buffer) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLenum, GLuint)>("glBindBuffer");
#ifdef __EMSCRIPTEN__
    if (target == 0x8892) s_arrayBuffer = buffer;
    if (target == 0x8893) s_elementBuffer = buffer;
#endif
    real(target, buffer);
}

void glBufferData(GLenum target, int size, const void* data, GLenum usage) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLenum, GLsizeiptr, const void*, GLenum)>("glBufferData");
#ifdef __EMSCRIPTEN__
    if (target == 0x8893 && s_elementBuffer) {
        const unsigned char* bytes = static_cast<const unsigned char*>(data);
        std::vector<unsigned char>& copy = s_elements[s_elementBuffer];
        if (bytes) copy.assign(bytes, bytes + size);
        else copy.assign(size_t(size), 0);
    }
#endif
    real(target, static_cast<GLsizeiptr>(size), data, usage);
}

void glBufferSubData(GLenum target, int offset, int size, const void* data) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLenum, GLintptr, GLsizeiptr, const void*)>("glBufferSubData");
#ifdef __EMSCRIPTEN__
    auto copy = target == 0x8893 ? s_elements.find(s_elementBuffer) : s_elements.end();
    if (copy != s_elements.end() && offset >= 0 && size_t(offset) + size_t(size) <= copy->second.size())
        std::copy(static_cast<const unsigned char*>(data), static_cast<const unsigned char*>(data) + size,
                  copy->second.begin() + offset);
#endif
    real(target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLsizei, const GLuint*)>("glDeleteBuffers");
#ifdef __EMSCRIPTEN__
    for (GLsizei i = 0; i < n; i++) {
        s_elements.erase(buffers[i]);
        if (buffers[i] == s_arrayBuffer) s_arrayBuffer = 0;
        if (buffers[i] == s_elementBuffer) s_elementBuffer = 0;
    }
#endif
    real(n, buffers);
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
                           const void* pointer) {
    MEDECOMP_GL_GUARD();
    static auto real = proc<void (GL_APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)>(
        "glVertexAttribPointer");
#ifdef __EMSCRIPTEN__
    if (index < 32) {
        if (s_arrayBuffer) s_client &= ~(1u << index);
        else s_client |= 1u << index;
    }
#endif
    real(index, size, type, normalized, stride, pointer);
}

}
