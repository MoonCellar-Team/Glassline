#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <vector>

#include "../gfx/texture_pack.h"
#include "gl_shared.h"
#include "game/AppEngine.h"
#include "game/GameObjectPlayer.h"
#include "game/GameScreen.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"
#include "m3g/Appearance.h"
#include "m3g/Camera.h"
#include "m3g/Image2D.h"
#include "m3g/Image2DPlatformData.h"
#include "m3g/IndexBuffer.h"
#include "m3g/Mesh.h"
#include "m3g/Node.h"
#include "m3g/Texture2D.h"
#include "m3g/Transform.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "m3g/World.h"
#include "m3g/m3g.h"
#include "port/platform.h"

namespace port {
namespace platform {

namespace {

const m3g::Node* s_dome;
bool s_found;
float s_dir[3];

void readArray(m3g::VertexArray* array, std::vector<float>* out) {
    int n = array->getVertexCount() * array->getComponentCount();
    out->resize(size_t(n));
    if (array->getComponentType() == 1) {
        std::vector<signed char> v(static_cast<size_t>(n));
        array->get(0, array->getVertexCount(), v.data());
        for (int i = 0; i < n; i++) (*out)[size_t(i)] = v[size_t(i)];
    } else if (array->getComponentType() == 2) {
        std::vector<short> v(static_cast<size_t>(n));
        array->get(0, array->getVertexCount(), v.data());
        for (int i = 0; i < n; i++) (*out)[size_t(i)] = v[size_t(i)];
    } else {
        array->get(0, array->getVertexCount(), out->data());
    }
}

void meshesOf(m3g::Node* node, std::vector<m3g::Mesh*>* out) {
    if (m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node)) out->push_back(mesh);
    if (m3g::Group* group = m3g::m3g_cast<m3g::Group>(node))
        for (int i = 0; i < group->getChildCount(); i++) meshesOf(group->getChild(i), out);
}

bool pixels(m3g::Image2D* image, std::vector<uint8_t>* rgba) {
    const char* why = "";
    if (gfx::imageRGBA(image, rgba, &why)) return true;
    if (!image->m_platform || !image->m_platform->texture) return false;
    static GLuint fbo;
    if (!fbo) gl.GenFramebuffers(1, &fbo);
    GLint was;
    gl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &was);
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GLuint(image->m_platform->texture), 0);
    bool ok = gl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (ok) {
        rgba->resize(size_t(image->getWidth()) * size_t(image->getHeight()) * 4);
        gl.ReadPixels(0, 0, image->getWidth(), image->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, rgba->data());
    }
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    gl.BindFramebuffer(GL_FRAMEBUFFER, GLuint(was));
    return ok;
}

bool sunSpot(m3g::Image2D* image, float* s, float* t) {
    std::vector<uint8_t> rgba;
    if (!image || !pixels(image, &rgba)) return false;
    const int G = 128, R = 4;
    int w = image->getWidth(), h = image->getHeight();
    std::vector<float> grid(G * G, 0.0f), tmp(G * G);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const uint8_t* p = &rgba[(size_t(y) * size_t(w) + size_t(x)) * 4];
            grid[size_t(y * G / h * G + x * G / w)] += (p[0] + p[1] + p[2]) * p[3];
        }
    for (int pass = 0; pass < 4; pass++) {
        bool alongX = pass % 2 == 0;
        for (int y = 0; y < G; y++)
            for (int x = 0; x < G; x++) {
                float sum = 0;
                for (int k = -R; k <= R; k++) {
                    int xx = alongX ? std::min(G - 1, std::max(0, x + k)) : x;
                    int yy = alongX ? y : std::min(G - 1, std::max(0, y + k));
                    sum += grid[size_t(yy * G + xx)];
                }
                tmp[size_t(y * G + x)] = sum;
            }
        grid.swap(tmp);
    }
    float peak = *std::max_element(grid.begin(), grid.end());
    if (peak <= 0) return false;
    double sx = 0, sy = 0;
    int n = 0;
    for (int y = 0; y < G; y++)
        for (int x = 0; x < G; x++)
            if (grid[size_t(y * G + x)] >= peak * 0.98f) {
                sx += x + 0.5;
                sy += y + 0.5;
                n++;
            }
    *s = float(sx / n / G);
    *t = float(sy / n / G);
    return true;
}

bool locate(m3g::Mesh* mesh, m3g::Texture2D* texture, float s, float t, float* pos) {
    m3g::VertexBuffer* vb = mesh->getVertexBuffer();
    float psb[4], tsb[4];
    m3g::VertexArray* positions = vb ? vb->getPositions(psb) : nullptr;
    m3g::VertexArray* uvs = vb ? vb->getTexCoords(0, tsb) : nullptr;
    if (!positions || !uvs) return false;
    std::vector<float> p, uv;
    readArray(positions, &p);
    readArray(uvs, &uv);
    int n = vb->getVertexCount(), comps = uvs->getComponentCount();
    m3g::Transform tt;
    texture->getCompositeTransform(&tt);
    std::vector<float> st(size_t(n) * 2);
    for (int i = 0; i < n; i++) {
        float v[4] = {uv[size_t(i * comps)] * tsb[0] + tsb[1], uv[size_t(i * comps + 1)] * tsb[0] + tsb[2], 0, 1};
        tt.transform(v, 4);
        st[size_t(i) * 2] = v[0] / v[3];
        st[size_t(i) * 2 + 1] = v[1] / v[3];
    }
    for (int sub = 0; sub < mesh->getSubmeshCount(); sub++) {
        m3g::IndexBuffer* ib = mesh->getIndexBuffer(sub);
        if (!ib) continue;
        midp::array<int> idx(ib->getIndexCount());
        ib->getIndices(idx);
        for (int i = 0; i + 2 < idx.m_length; i += 3) {
            const float* a = &st[size_t(idx[i]) * 2];
            const float* b = &st[size_t(idx[i + 1]) * 2];
            const float* c = &st[size_t(idx[i + 2]) * 2];
            float d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);
            if (fabsf(d) < 1e-12f) continue;
            float wa = ((b[1] - c[1]) * (s - c[0]) + (c[0] - b[0]) * (t - c[1])) / d;
            float wb = ((c[1] - a[1]) * (s - c[0]) + (a[0] - c[0]) * (t - c[1])) / d;
            float wc = 1 - wa - wb;
            if (wa < 0 || wb < 0 || wc < 0) continue;
            for (int k = 0; k < 3; k++)
                pos[k] = (wa * p[size_t(idx[i]) * 3 + size_t(k)] + wb * p[size_t(idx[i + 1]) * 3 + size_t(k)] +
                          wc * p[size_t(idx[i + 2]) * 3 + size_t(k)]) * psb[0] + psb[1 + k];
            return true;
        }
    }
    return false;
}

void findSun(m3g::Node* dome) {
    s_dome = dome;
    s_found = false;
    std::vector<m3g::Mesh*> meshes;
    meshesOf(dome, &meshes);
    for (m3g::Mesh* mesh : meshes) {
        m3g::Appearance* appearance = mesh->getAppearance(0);
        m3g::Texture2D* texture = appearance ? appearance->getTexture(0) : nullptr;
        float s, t, pos[3];
        if (!texture || !sunSpot(texture->getImage(), &s, &t) || !locate(mesh, texture, s, t, pos)) continue;
        float len = sqrtf(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
        if (len <= 0) continue;
        for (int k = 0; k < 3; k++) s_dir[k] = pos[k] / len;
        s_found = true;
        fprintf(stderr, "medecomp: солнце: пятно (%.3f, %.3f) на текстуре купола, направление (%.2f, %.2f, %.2f)\n", s,
                t, s_dir[0], s_dir[1], s_dir[2]);
        break;
    }
    if (!s_found) fprintf(stderr, "medecomp: солнце: на куполе не найдено (сеток %zu)\n", meshes.size());
}

}

bool sunInWorld(float out[3]) {
    AppEngine* app = AppEngine::getCanvas();
    SceneGame* scene = app ? app->getSceneGame() : nullptr;
    m3g::Node* dome = scene ? scene->getMap().getSkyDomeNode() : nullptr;
    if (!dome) return false;
    if (dome != s_dome) findSun(dome);
    if (!s_found) return false;
    m3g::Node* root = dome;
    while (root->getParent()) root = root->getParent();
    m3g::Transform t;
    if (!dome->getTransformTo(root, &t)) return false;
    float v[4] = {s_dir[0], s_dir[1], s_dir[2], 0};
    t.transform(v, 4);
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len <= 0) return false;
    for (int k = 0; k < 3; k++) out[k] = v[k] / len;
    return true;
}

bool sunInCamera(float out[3]) {
    AppEngine* app = AppEngine::getCanvas();
    SceneGame* scene = app ? app->getSceneGame() : nullptr;
    m3g::Node* dome = scene ? scene->getMap().getSkyDomeNode() : nullptr;
    if (!dome) return false;
    if (dome != s_dome) findSun(dome);
    if (!s_found) return false;
    m3g::Node* root = dome;
    while (root->getParent()) root = root->getParent();
    m3g::World* world = root->getM3GUniqueClassID() == 22 ? static_cast<m3g::World*>(root) : nullptr;
    m3g::Camera* camera = world ? world->getActiveCamera() : nullptr;
    m3g::Transform toWorld, fromCamera;
    if (!camera || !dome->getTransformTo(world, &toWorld) || !camera->getTransformTo(world, &fromCamera)) return false;
    fromCamera.invert();
    fromCamera.postMultiply(&toWorld);
    float v[4] = {s_dir[0], s_dir[1], s_dir[2], 0};
    fromCamera.transform(v, 4);
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len <= 0) return false;
    for (int k = 0; k < 3; k++) out[k] = v[k] / len;
    return true;
}

bool gameCamera(float cameraToWorld[16], float* heroineSpeed) {
    AppEngine* app = AppEngine::getCanvas();
    if (!app || app->m_sceneId != 3) return false;
    SceneGame* scene = app->getSceneGame();
    if (!scene || scene->m_loadingStep < 13 || scene->m_gameScreens.m_length <= 0) return false;
    GameScreen* screen = scene->getGameScreen(0);
    m3g::World* world = screen ? static_cast<m3g::World*>(screen->m_world) : nullptr;
    m3g::Camera* camera = screen ? static_cast<m3g::Camera*>(screen->m_m3gCamera) : nullptr;
    GameObjectPlayer* player = screen ? screen->getPlayerObject() : nullptr;
    m3g::Transform t;
    if (!world || !camera || !player || !camera->getTransformTo(world, &t)) return false;
    t.get(cameraToWorld);
    const MathVector& v = player->m_velocity;
    *heroineSpeed = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return true;
}

}
}
