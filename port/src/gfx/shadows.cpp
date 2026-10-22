#include "shadows.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <map>
#include <string>

#include "../platform/settings.h"
#include "gl_proc.h"
#include "m3g/CompositingMode.h"
#include "m3g/IndexBuffer.h"
#include "m3g/OpenGLES20Renderer.h"
#include "m3g/OpenGLES20ShaderSupport.h"
#include "m3g/PolygonMode.h"
#include "m3g/RenderNode.h"
#include "m3g/RendererSupport.h"
#include "m3g/SkinnedMesh.h"
#include "m3g/Transform.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "m3g/m3g.h"
#include "port/platform.h"
#include "runtime/midp/GLES20.h"

extern "C" {
unsigned glCreateShader(unsigned);
void glShaderSource(unsigned, int, const char* const*, const int*);
void glCompileShader(unsigned);
void glGetShaderiv(unsigned, unsigned, int*);
void glGetShaderInfoLog(unsigned, int, int*, char*);
unsigned glCreateProgram();
void glAttachShader(unsigned, unsigned);
void glBindAttribLocation(unsigned, unsigned, const char*);
void glLinkProgram(unsigned);
void glGetProgramiv(unsigned, unsigned, int*);
int glGetUniformLocation(unsigned, const char*);
void glGetIntegerv(unsigned, int*);
unsigned char glIsEnabled(unsigned);
void glTexParameteri(unsigned, unsigned, int);
}

const char* const medecomp_shadow_vertex_header = "uniform highp mat4 g_ShadowMatrix;\nvarying highp vec3 v_Shadow;\n";
const char* const medecomp_shadow_vertex_footer = "\tv_Shadow = (g_ShadowMatrix * vec4(vs_Position, 1.0)).xyz;\n";
const char* const medecomp_shadow_pixel_header =
    "uniform highp sampler2D g_ShadowMap;\n"
    "uniform highp vec3 g_ShadowEye;\n"
    "uniform highp float g_ShadowTexel;\n"
    "varying highp vec3 v_Shadow;\n"
    "float RunnerShadow()\n{\n"
    "\thighp vec3 n = cross(dFdx(v_Shadow), dFdy(v_Shadow));\n"
    "\tif (v_Shadow.x <= 0.0 || v_Shadow.x >= 1.0 || v_Shadow.y <= 0.0 || v_Shadow.y >= 1.0 || v_Shadow.z >= 1.0)\n"
    "\t\treturn 0.0;\n"
    "\thighp float side = dot(n, g_ShadowEye - v_Shadow) > 0.0 ? -1.0 : 1.0;\n"
    "\thighp float facing = side * n.z / max(length(vec3(n.xy * (60.0 / 14.0), n.z)), 1e-30);\n"
    "\tfloat lit = smoothstep(0.02, 0.12, facing);\n"
    "\tif (lit <= 0.0)\n"
    "\t\treturn 0.0;\n"
    "\tfloat s = 0.0;\n"
    "\tfor (int y = 0; y < 4; y++)\n"
    "\t\tfor (int x = 0; x < 4; x++)\n"
    "\t\t\ts += step(texture2D(g_ShadowMap, v_Shadow.xy + (vec2(float(x), float(y)) - 1.5) * g_ShadowTexel).r,"
    " v_Shadow.z - 0.0005);\n"
    "\treturn s / 16.0 * lit;\n}\n";
const char* const medecomp_shadow_pixel_after = "\tcolour.rgb *= 1.0 - 0.45 * RunnerShadow();\n";

namespace port {
namespace gfx {

namespace {

#ifdef __ANDROID__
const int kSize = 2048;
#else
const int kSize = 4096;
#endif
const int kSizeMin = 2048;
const float kHalf = 14.0f;
const float kDepth = 60.0f;
const float kAhead = 9.0f;
const unsigned kShadowUnit = 3;

const char* kCasterVertex =
    "attribute highp vec3 a_pos;\n"
    "uniform highp mat4 u_mvp;\n"
    "void main() { gl_Position = u_mvp * vec4(a_pos, 1.0); }\n";
const char* kCasterFragment = "precision mediump float;\nvoid main() { gl_FragColor = vec4(1.0); }\n";

bool s_tried, s_ok, s_active;
int s_size = kSize;
unsigned s_texture, s_fbo, s_program;
int s_mvp = -1;
float s_toNdc[16];
float s_toMap[16];
float s_eye[3];
nio::FloatBuffer* s_skin;
struct Locations {
    int matrix, map, eye, texel;
};
std::map<int, Locations> s_locations;

unsigned compile(unsigned type, const char* source) {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int ok = 0;
    glGetShaderiv(shader, 0x8b81, &ok);
    if (!ok) {
        char log[512] = "";
        glGetShaderInfoLog(shader, sizeof log, nullptr, log);
        fprintf(stderr, "medecomp: тени: шейдер не собран: %s\n", log);
    }
    return ok ? shader : 0;
}

bool create(midp::GLES20* gl) {
    s_tried = true;
    static auto getString = reinterpret_cast<const char* (*)(unsigned)>(glProc("glGetString"));
    const char* version = getString(0x1f02);
    int major = 0;
    if (version) sscanf(version, "OpenGL ES %d", &major);
    if (major < 3) {
        fprintf(stderr, "medecomp: тени от бегунов — нужен OpenGL ES 3.0, выключены\n");
        return false;
    }
    unsigned vs = compile(0x8b31, kCasterVertex), fs = compile(0x8b30, kCasterFragment);
    if (!vs || !fs) return false;
    s_program = glCreateProgram();
    glAttachShader(s_program, vs);
    glAttachShader(s_program, fs);
    glBindAttribLocation(s_program, 0, "a_pos");
    glLinkProgram(s_program);
    int linked = 0;
    glGetProgramiv(s_program, 0x8b82, &linked);
    s_mvp = glGetUniformLocation(s_program, "u_mvp");
    if (!linked || s_mvp < 0) return false;
    int active = 0, fbo = 0, maxSize = 0;
    glGetIntegerv(0x84e0, &active);
    glGetIntegerv(0x8ca6, &fbo);
    glGetIntegerv(0x0d33, &maxSize);
    s_size = maxSize >= kSize ? kSize : kSizeMin;
    bool complete = false;
    for (;;) {
        gl->glActiveTexture(0x84c0 + kShadowUnit);
        gl->glGenTextures(1, &s_texture);
        gl->glBindTexture(0xde1, s_texture);
        gl->glTexImage2D(0xde1, 0, 0x81a6, s_size, s_size, 0, 0x1902, 0x1405, nullptr);
        glTexParameteri(0xde1, 0x2801, 0x2600);
        glTexParameteri(0xde1, 0x2800, 0x2600);
        glTexParameteri(0xde1, 0x2802, 0x812f);
        glTexParameteri(0xde1, 0x2803, 0x812f);
        gl->glActiveTexture(active);
        gl->glGenFramebuffers(1, &s_fbo);
        gl->glBindFramebuffer(0x8d40, s_fbo);
        gl->glFramebufferTexture2D(0x8d40, 0x8d00, 0xde1, s_texture, 0);
        complete = gl->glCheckFramebufferStatus(0x8d40) == 0x8cd5;
        gl->glBindFramebuffer(0x8d40, fbo);
        if (complete || s_size <= kSizeMin) break;
        gl->glDeleteFramebuffers(1, &s_fbo);
        gl->glDeleteTextures(1, &s_texture);
        s_size = kSizeMin;
    }
    if (!complete) fprintf(stderr, "medecomp: тени: буфер карты не собран\n");
    else fprintf(stderr, "medecomp: тени от бегунов, карта %d²\n", s_size);
    return complete;
}

void setRow(float* m, int row, float x, float y, float z, float w) {
    m[row * 4] = x;
    m[row * 4 + 1] = y;
    m[row * 4 + 2] = z;
    m[row * 4 + 3] = w;
}

void lightMatrices(const float* view) {
    m3g::Transform camera;
    camera.set(view);
    camera.invert();
    float c[16];
    camera.get(c);
    float center[3] = {c[3] - c[2] * kAhead, c[7] - c[6] * kAhead, c[11] - c[10] * kAhead};
    float z[3] = {0.0f, 1.0f, 0.0f};
    port::platform::sunInWorld(z);
    float up[3] = {0.0f, 1.0f, 0.0f};
    if (fabsf(z[1]) > 0.99f) up[0] = 1.0f, up[1] = 0.0f;
    float x[3] = {up[1] * z[2] - up[2] * z[1], up[2] * z[0] - up[0] * z[2], up[0] * z[1] - up[1] * z[0]};
    float lx = sqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    for (float& v : x) v /= lx;
    float y[3] = {z[1] * x[2] - z[2] * x[1], z[2] * x[0] - z[0] * x[2], z[0] * x[1] - z[1] * x[0]};
    float texel = 2.0f * kHalf / float(s_size);
    float cx = floorf((x[0] * center[0] + x[1] * center[1] + x[2] * center[2]) / texel) * texel;
    float cy = floorf((y[0] * center[0] + y[1] * center[1] + y[2] * center[2]) / texel) * texel;
    float cz = z[0] * center[0] + z[1] * center[1] + z[2] * center[2];
    setRow(s_toNdc, 0, x[0] / kHalf, x[1] / kHalf, x[2] / kHalf, -cx / kHalf);
    setRow(s_toNdc, 1, y[0] / kHalf, y[1] / kHalf, y[2] / kHalf, -cy / kHalf);
    setRow(s_toNdc, 2, -z[0] / kDepth, -z[1] / kDepth, -z[2] / kDepth, cz / kDepth);
    setRow(s_toNdc, 3, 0, 0, 0, 1);
    for (int i = 0; i < 12; i++) s_toMap[i] = s_toNdc[i] * 0.5f;
    s_toMap[3] += 0.5f, s_toMap[7] += 0.5f, s_toMap[11] += 0.5f;
    setRow(s_toMap, 3, 0, 0, 0, 1);
    for (int r = 0; r < 3; r++)
        s_eye[r] = s_toMap[r * 4] * c[3] + s_toMap[r * 4 + 1] * c[7] + s_toMap[r * 4 + 2] * c[11] + s_toMap[r * 4 + 3];
}

void upload(midp::GLES20* gl, int location, const float* rowMajor, const m3g::Transform* model) {
    m3g::Transform t;
    t.set(rowMajor);
    t.postMultiply(model);
    float m[16], columns[16];
    t.get(m);
    for (int r = 0; r < 4; r++)
        for (int k = 0; k < 4; k++) columns[k * 4 + r] = m[r * 4 + k];
    gl->glUniformMatrix4fv(location, 1, 0, columns);
}

}

bool runnerShadowsSetting() { return platform::settings::get("runner_shadows", "off") == "on"; }

void setRunnerShadowsSetting(bool on) { platform::settings::set("runner_shadows", on ? "on" : "off"); }

}
}

using namespace port::gfx;

bool medecomp_shadows_enabled(void) {
    static const bool enabled = runnerShadowsSetting() && !port::platform::originalGraphics();
    return enabled;
}

void medecomp_shadow_pass(const m3g::RenderNode* nodes, int count, m3g::Renderer* base) {
    s_active = false;
    if (!medecomp_shadows_enabled()) return;
    int casters = 0;
    for (int i = 0; i < count; i++) casters += nodes[i].m_skinned && nodes[i].m_appearance;
    if (!casters) return;
    m3g::OpenGLES20Renderer* renderer = static_cast<m3g::OpenGLES20Renderer*>(base);
    midp::GLES20* gl = renderer->m_gl;
    if (!s_tried) s_ok = create(gl);
    if (!s_ok) return;
    float view[16];
    renderer->m_view.get(view);
    lightMatrices(view);

    int fbo = 0, viewport[4];
    glGetIntegerv(0x8ca6, &fbo);
    glGetIntegerv(0x0ba2, viewport);
    bool scissor = glIsEnabled(0x0c11);
    gl->glBindFramebuffer(0x8d40, s_fbo);
    gl->glViewport(0, 0, s_size, s_size);
    gl->glDisable(0x0c11);
    gl->glDisable(0x0be2);
    gl->glDisable(0x0b44);
    gl->glDisable(0x8037);
    gl->glEnable(0x0b71);
    gl->glDepthMask(1);
    gl->glColorMask(0, 0, 0, 0);
    glClear(0x100);
    gl->glUseProgram(s_program);
    gl->glBindBuffer(0x8892, 0);
    for (unsigned a = 1; a <= 6; a++) gl->glDisableVertexAttribArray(a);
    gl->glEnableVertexAttribArray(0);
    for (int i = 0; i < count; i++) {
        const m3g::RenderNode& item = nodes[i];
        if (!item.m_skinned || !item.m_appearance || !item.m_vertices || !item.m_indices) continue;
        m3g::SkinnedMesh* mesh = m3g::m3g_cast<m3g::SkinnedMesh>(const_cast<m3g::Node*>(item.m_node));
        float scaleBias[4];
        m3g::VertexArray* positions = item.m_vertices->getPositions(scaleBias);
        if (!mesh || !positions || !mesh->getSkinIndices() || !mesh->getSkinWeights() || !mesh->getBoneTransforms())
            continue;
        m3g::softwareSkin(s_skin, positions, mesh->getSkinIndices(), mesh->getSkinWeights(), mesh->getBoneTransforms(),
                          scaleBias, true);
        float* data = s_skin->lockBuffer();
        glVertexAttribPointer(0, 3, 0x1406, 0, 0, data);
        s_skin->unlock();
        upload(gl, s_mvp, s_toNdc, item.m_transform);
        m3g::drawIndexBuffer(gl, item.m_indices);
    }
    gl->glBindFramebuffer(0x8d40, fbo);
    gl->glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    if (scissor) gl->glEnable(0x0c11);
    int active = 0;
    glGetIntegerv(0x84e0, &active);
    gl->glActiveTexture(0x84c0 + kShadowUnit);
    gl->glBindTexture(0xde1, s_texture);
    gl->glActiveTexture(active);
    renderer->m_vertices = nullptr;
    renderer->m_appearance = nullptr;
    renderer->m_compositing = reinterpret_cast<m3g::CompositingMode*>(-1);
    renderer->m_polygon = reinterpret_cast<m3g::PolygonMode*>(-1);
    renderer->m_shaderSupport->notifyCacheInvalidation();
    s_active = true;
}

void medecomp_shadow_end(void) { s_active = false; }

bool medecomp_shadows_active(void) { return s_active; }

void medecomp_shadow_receive(midp::GLES20* gl, int program, const m3g::Transform* model) {
    if (!medecomp_shadows_enabled()) return;
    auto it = s_locations.find(program);
    if (it == s_locations.end()) {
        unsigned p = unsigned(program);
        it = s_locations
                 .insert({program, {glGetUniformLocation(p, "g_ShadowMatrix"), glGetUniformLocation(p, "g_ShadowMap"),
                                    glGetUniformLocation(p, "g_ShadowEye"), glGetUniformLocation(p, "g_ShadowTexel")}})
                 .first;
        gl->glUniform1i(it->second.map, int(kShadowUnit));
        if (it->second.texel >= 0) gl->glUniform1f(it->second.texel, 1.0f / float(s_size));
    }
    if (it->second.matrix < 0) return;
    if (s_active) {
        upload(gl, it->second.matrix, s_toMap, model);
        if (it->second.eye >= 0) gl->glUniform3f(it->second.eye, s_eye[0], s_eye[1], s_eye[2]);
    } else {
        static const float zero[16] = {};
        gl->glUniformMatrix4fv(it->second.matrix, 1, 0, zero);
    }
}
