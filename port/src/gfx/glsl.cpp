#include <stdlib.h>
#include <string.h>

#include <string>

#include "gl_proc.h"

typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef char GLchar;

#if defined(_WIN32)
#define GL_APIENTRY __stdcall
#else
#define GL_APIENTRY
#endif

namespace port {
namespace gfx {

namespace {

const GLenum kShaderType = 0x8B4F;
const GLenum kFragmentShader = 0x8B30;
const char kFragOut[] = "medecomp_FragColor";

bool identStart(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
bool identChar(char c) { return identStart(c) || (c >= '0' && c <= '9'); }

const char* replacement(const std::string& word, bool fragment) {
    if (word == "attribute") return fragment ? nullptr : "in";
    if (word == "varying") return fragment ? "in" : "out";
    if (word == "texture2D" || word == "textureCube") return "texture";
    if (word == "texture2DLod" || word == "textureCubeLod" || word == "texture2DLodEXT") return "textureLod";
    if (word == "texture2DProj") return "textureProj";
    if (word == "texture2DProjLod") return "textureProjLod";
    if (fragment && word == "gl_FragColor") return kFragOut;
    return nullptr;
}

}

bool glsl3Wanted() {
#ifdef __EMSCRIPTEN__
    return true;
#else
    static const bool on = getenv("MEDECOMP_GLSL3") && getenv("MEDECOMP_GLSL3")[0] == '1';
    return on;
#endif
}

std::string toGlsl3(const std::string& src, bool fragment) {
    std::string out = "#version 300 es\n";
    if (fragment) out += std::string("out highp vec4 ") + kFragOut + ";\n";
    size_t i = 0;
    bool lineStart = true;
    while (i < src.size()) {
        char c = src[i];
        if (lineStart && c == '#') {
            size_t end = src.find('\n', i);
            std::string line = src.substr(i, end == std::string::npos ? std::string::npos : end - i);
            if (line.find("GL_OES_standard_derivatives") == std::string::npos && line.compare(0, 8, "#version") != 0)
                out += line;
            if (end == std::string::npos) break;
            out += '\n';
            i = end + 1;
            continue;
        }
        if (c == '/' && i + 1 < src.size() && (src[i + 1] == '/' || src[i + 1] == '*')) {
            size_t end = src[i + 1] == '/' ? src.find('\n', i) : src.find("*/", i + 2);
            end = end == std::string::npos ? src.size() : end + (src[i + 1] == '*' ? 2 : 0);
            out.append(src, i, end - i);
            i = end;
            continue;
        }
        if (identStart(c)) {
            size_t j = i;
            while (j < src.size() && identChar(src[j])) j++;
            std::string word = src.substr(i, j - i);
            if (fragment && word == "gl_FragData") {
                size_t k = src.find(']', j);
                if (k != std::string::npos && src.compare(j, k + 1 - j, "[0]") == 0) {
                    out += kFragOut;
                    i = k + 1;
                    lineStart = false;
                    continue;
                }
            }
            const char* r = replacement(word, fragment);
            out += r ? r : word.c_str();
            i = j;
            lineStart = false;
            continue;
        }
        out += c;
        if (c == '\n')
            lineStart = true;
        else if (c != ' ' && c != '\t')
            lineStart = false;
        i++;
    }
    return out;
}

}
}

extern "C" {

void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    MEDECOMP_GL_GUARD();
    using ShaderSource = void(GL_APIENTRY*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
    using GetShaderiv = void(GL_APIENTRY*)(GLuint, GLenum, GLint*);
    static auto real = reinterpret_cast<ShaderSource>(port::gfx::glProc("glShaderSource"));
    if (!port::gfx::glsl3Wanted()) return real(shader, count, string, length);
    std::string src;
    for (GLsizei n = 0; n < count; n++) {
        if (!string[n]) continue;
        if (length && length[n] >= 0)
            src.append(string[n], size_t(length[n]));
        else
            src += string[n];
    }
    if (src.find("#version") != std::string::npos) return real(shader, count, string, length);
    static auto getShaderiv = reinterpret_cast<GetShaderiv>(port::gfx::glProc("glGetShaderiv"));
    GLint type = 0;
    getShaderiv(shader, port::gfx::kShaderType, &type);
    std::string es3 = port::gfx::toGlsl3(src, GLenum(type) == port::gfx::kFragmentShader);
    const GLchar* p = es3.c_str();
    real(shader, 1, &p, nullptr);
}

}
