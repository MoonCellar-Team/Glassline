#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include "../gfx/png.h"
#include "game/AppEngine.h"
#include "game/GameCamera.h"
#include "game/GameObjectPlayer.h"
#include "game/GameScreen.h"
#include "game/LevelData.h"
#include "game/MEdgeMap.h"
#include "game/MapPalette.h"
#include "game/MathFrustum.h"
#include "game/SceneGame.h"
#include "m3g/Background.h"
#include "m3g/Camera.h"
#include "m3g/Graphics3D.h"
#include "m3g/Mesh.h"
#include "m3g/Node.h"
#include "m3g/Transform.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "m3g/World.h"
#include "m3g/m3g.h"
#include "port/files.h"
#include "port/platform.h"
#include "runtime/midp/Graphics.h"
#include "piece_surface.h"
#include "skins.h"

extern "C" void medecomp_sky_follow(m3g::Node* dome);
namespace {

const int kSceneMenu = 2, kSceneGame = 3;
const int kSettleFrames = 30;
const float kFov = 30.0f;
const int kPieceMax = 512;
const float kSideDepth = 2.0f;
const float kTopDepth = 1.0f;
const int kSideColor = 0xffff00ff;

enum Phase { kWait, kBlack, kWhite, kCombine, kLeft, kRight, kTop, kFrontBlack, kFrontWhite, kViewBlack, kViewWhite, kDone };

SceneGame* loadedLevel(int* settle) {
    AppEngine* app = AppEngine::getCanvas();
    SceneGame* scene = app && app->m_sceneId == kSceneGame ? app->getSceneGame() : nullptr;
    if (!scene || scene->m_loadingStep < 13 || scene->m_gameScreens.m_length <= 0) return nullptr;
    return ++*settle >= kSettleFrames ? scene : nullptr;
}

Phase s_phase = kWait;
int s_settle = 0;
size_t s_index = 0;
std::vector<int> s_ids;
std::string s_dir;
MapPalette* s_palette;
midp::ReferenceCountedPointer<m3g::World> s_world(NULL);
midp::ReferenceCountedPointer<m3g::Group> s_holder(NULL);
midp::ReferenceCountedPointer<m3g::Background> s_background(NULL);
midp::ReferenceCountedPointer<m3g::Camera> s_camera(NULL);
std::ofstream s_bounds, s_sides, s_surface, s_views;
float s_lo[3], s_hi[3];
port::platform::PiecePlanes s_planes;
float s_left, s_right;

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

void bounds(m3g::Node* node, m3g::Node* root, float* lo, float* hi) {
    if (m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node)) {
        m3g::VertexBuffer* vb = mesh->getVertexBuffer();
        float sb[4];
        m3g::VertexArray* positions = vb ? vb->getPositions(sb) : nullptr;
        m3g::Transform t;
        if (positions && positions->getComponentCount() == 3 && mesh->getTransformTo(root, &t)) {
            std::vector<float> p;
            readArray(positions, &p);
            for (size_t i = 0; i + 2 < p.size(); i += 3) {
                float v[4] = {p[i] * sb[0] + sb[1], p[i + 1] * sb[0] + sb[2], p[i + 2] * sb[0] + sb[3], 1};
                t.transform(v, 4);
                for (int k = 0; k < 3; k++) lo[k] = std::min(lo[k], v[k]), hi[k] = std::max(hi[k], v[k]);
            }
        }
    }
    if (m3g::Group* group = m3g::m3g_cast<m3g::Group>(node))
        for (int i = 0; i < group->getChildCount(); i++) bounds(group->getChild(i), root, lo, hi);
}

[[noreturn]] void finish(int code) {
#ifdef MEDECOMP_ONE_PROCESS
    port::platform::appRestart();
#endif
    _exit(code);
}

bool setupPieces() {
    AppEngine* app = AppEngine::getCanvas();
    int palette = -1, modelSet = -1;
    const char* spec = getenv("MEDECOMP_PIECE_PALETTE");
    if (!spec || sscanf(spec, "%d,%d", &palette, &modelSet) != 2 || palette < 0 || modelSet < 0) return false;
    for (const char* p = getenv("MEDECOMP_PIECE_IDS"); p && *p;) {
        s_ids.push_back(atoi(p));
        p = strchr(p, ',');
        if (p) p++;
    }
    const ModelSet* models = AppEngine::getLevelData()->getModelSet(MEdgeMap::LEVEL_DATA_LOOKUP_ARRAY[modelSet]);
    std::thread([&] { s_palette = new MapPalette(MEdgeMap::RES_LOOKUP_ARRAY[palette], models); }).join();
    s_world = new m3g::World();
    s_camera = new m3g::Camera();
    s_camera->setPerspective(medecomp_wide_fov(kFov), medecomp_wide_aspect(float(app->getHeight())), 0.1f, 500.0f);
    s_world->addChild(s_camera);
    s_world->setActiveCamera(s_camera);
    s_background = new m3g::Background();
    s_background->setColorClearEnable(true);
    s_background->setDepthClearEnable(true);
    s_world->setBackground(s_background);
    s_holder = new m3g::Group();
    s_world->addChild(s_holder);
    s_bounds.open((s_dir + "/bounds.txt").c_str());
    s_sides.open((s_dir + "/sides.txt").c_str());
    s_surface.open((s_dir + "/surface.txt").c_str());
    s_views.open((s_dir + "/views.txt").c_str());
    return true;
}

bool placePiece(int id) {
    while (s_holder->getChildCount() > 0) s_holder->removeChild(s_holder->getChild(0));
    m3g::Node* node = s_palette->createUniqueNode(id);
    if (!node) return false;
    s_holder->addChild(node);
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    bounds(node, s_holder, lo, hi);
    if (lo[0] > hi[0]) return false;
    s_bounds << id << ' ' << lo[0] << ' ' << lo[1] << ' ' << lo[2] << ' ' << hi[0] << ' ' << hi[1] << ' ' << hi[2] << '\n';
    std::copy(lo, lo + 3, s_lo);
    std::copy(hi, hi + 3, s_hi);
    port::platform::PieceSurface surface;
    port::platform::readPieceSurface(node, &surface);
    s_planes = surface.planes;
    float c[3] = {(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2};
    float r = 0.5f * sqrtf((hi[0] - lo[0]) * (hi[0] - lo[0]) + (hi[1] - lo[1]) * (hi[1] - lo[1]) + (hi[2] - lo[2]) * (hi[2] - lo[2]));
    float d = std::max(1.0f, r) / tanf(kFov * 0.5f * 3.14159265f / 180.0f) * 1.15f;
    m3g::Transform t;
    t.postTranslate(c[0], c[1], c[2]);
    t.postRotate(-25.0f, 0.0f, 1.0f, 0.0f);
    t.postRotate(-15.0f, 1.0f, 0.0f, 0.0f);
    t.postTranslate(0.0f, 0.0f, d);
    t.postRotate(90.0f, 0.0f, 0.0f, 1.0f);
    s_camera->setTransform(&t);
    AppEngine* app = AppEngine::getCanvas();
    s_camera->setPerspective(medecomp_wide_fov(kFov), medecomp_wide_aspect(float(app->getHeight())), 0.1f, 500.0f);
    return true;
}

void sideCamera(bool left) {
    float h = std::max(0.01f, s_hi[1] - s_lo[1]), d = std::max(0.01f, s_hi[2] - s_lo[2]);
    m3g::Transform t;
    t.postTranslate(left ? s_lo[0] - 1.0f : s_hi[0] + 1.0f, (s_lo[1] + s_hi[1]) / 2, (s_lo[2] + s_hi[2]) / 2);
    t.postRotate(left ? -90.0f : 90.0f, 0.0f, 1.0f, 0.0f);
    s_camera->setTransform(&t);
    s_camera->setParallel(h, d / h, 0.5f, 1.0f + kSideDepth);
}

void topCamera() {
    float w = std::max(0.01f, s_hi[0] - s_lo[0]), d = std::max(0.01f, s_hi[2] - s_lo[2]);
    m3g::Transform t;
    t.postTranslate((s_lo[0] + s_hi[0]) / 2, s_hi[1] + 1.0f, (s_lo[2] + s_hi[2]) / 2);
    t.postRotate(-90.0f, 1.0f, 0.0f, 0.0f);
    s_camera->setTransform(&t);
    s_camera->setParallel(d, w / d, 0.5f, 1.0f + kTopDepth);
}

void frontCamera() {
    const float* lo = s_planes.faceLo;
    const float* hi = s_planes.faceHi;
    float w = std::max(0.01f, hi[0] - lo[0]), h = std::max(0.01f, hi[1] - lo[1]);
    m3g::Transform t;
    t.postTranslate((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, s_hi[2] + 1.0f);
    t.postRotate(90.0f, 0.0f, 0.0f, 1.0f);
    s_camera->setTransform(&t);
    s_camera->setParallel(w, h / w, 0.5f, 1.0f + s_hi[2] - (s_planes.front - port::platform::kFrontBand));
}

void viewCamera() {
    float w = std::max(0.01f, s_hi[0] - s_lo[0]), h = std::max(0.01f, s_hi[1] - s_lo[1]);
    m3g::Transform t;
    t.postTranslate((s_lo[0] + s_hi[0]) / 2, (s_lo[1] + s_hi[1]) / 2, s_hi[2] + 1.0f);
    t.postRotate(90.0f, 0.0f, 0.0f, 1.0f);
    s_camera->setTransform(&t);
    s_camera->setParallel(w, h / w, 0.5f, 1.5f + s_hi[2] - s_lo[2]);
}

float sideCover(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    port::gfx::Image im;
    if (bytes.empty() || !port::gfx::decodePNG(bytes.data(), bytes.size(), im, false) || im.width <= 0 || im.height <= 0)
        return -1.0f;
    size_t n = size_t(im.width) * size_t(im.height), drawn = 0;
    const int r = kSideColor >> 16 & 0xff, g = kSideColor >> 8 & 0xff, b = kSideColor & 0xff;
    for (size_t i = 0; i < n; i++) {
        const uint8_t* p = &im.rgba[i * 4];
        drawn += abs(p[0] - r) + abs(p[1] - g) + abs(p[2] - b) > 24;
    }
    return float(drawn) / float(n);
}

float opaqueShare(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    port::gfx::Image im;
    if (bytes.empty() || !port::gfx::decodePNG(bytes.data(), bytes.size(), im, false) || im.width <= 0 || im.height <= 0)
        return -1.0f;
    size_t n = size_t(im.width) * size_t(im.height), drawn = 0;
    for (size_t i = 0; i < n; i++) drawn += im.rgba[i * 4 + 3] > 128;
    return float(drawn) / float(n);
}

void shrink(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    port::gfx::Image im;
    if (bytes.empty() || !port::gfx::decodePNG(bytes.data(), bytes.size(), im, false)) return;
    int big = std::max(im.width, im.height);
    if (big <= kPieceMax) return;
    int w = std::max(1, im.width * kPieceMax / big), h = std::max(1, im.height * kPieceMax / big);
    std::vector<uint8_t> out(size_t(w) * size_t(h) * 4);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int sx0 = x * im.width / w, sx1 = std::max(sx0 + 1, (x + 1) * im.width / w);
            int sy0 = y * im.height / h, sy1 = std::max(sy0 + 1, (y + 1) * im.height / h);
            float sum[4] = {};
            for (int sy = sy0; sy < sy1; sy++)
                for (int sx = sx0; sx < sx1; sx++) {
                    const uint8_t* p = &im.rgba[(size_t(sy) * size_t(im.width) + size_t(sx)) * 4];
                    float a = p[3] / 255.0f;
                    for (int k = 0; k < 3; k++) sum[k] += p[k] * a;
                    sum[3] += a;
                }
            uint8_t* d = &out[(size_t(y) * size_t(w) + size_t(x)) * 4];
            for (int k = 0; k < 3; k++) d[k] = sum[3] > 0 ? uint8_t(std::min(255.0f, sum[k] / sum[3] + 0.5f)) : 0;
            d[3] = uint8_t(sum[3] / float((sx1 - sx0) * (sy1 - sy0)) * 255.0f + 0.5f);
        }
    port::gfx::writePNG(path, w, h, out.data());
}

void render(midp::Graphics* g, int color) {
    AppEngine* app = AppEngine::getCanvas();
    s_background->setColor(color);
    m3g::Graphics3D* g3d = app->getGraphics3D();
    g3d->bindTarget(g);
    g3d->setViewport(0, 0, app->getWidth(), app->getHeight());
    g3d->render(s_world);
    g3d->releaseTarget();
}

bool piecePreviews(midp::Graphics* g, const char* dir) {
    if (s_phase == kWait) {
        AppEngine* app = AppEngine::getCanvas();
        if (!app || app->m_sceneId != kSceneMenu || !AppEngine::getM3GAssets()) return false;
        if (++s_settle < kSettleFrames) return false;
        s_dir = dir;
        if (!setupPieces()) {
            fprintf(stderr, "medecomp: превью кусков — нет MEDECOMP_PIECE_PALETTE\n");
            finish(1);
        }
        s_phase = kBlack;
    }
    if (s_index >= s_ids.size()) {
        s_bounds.close();
        s_sides.close();
        s_surface.close();
        s_views.close();
        const char* stamp = getenv("MEDECOMP_PIECE_STAMP");
        std::ofstream((s_dir + "/stamp").c_str(), std::ios::binary) << (stamp ? std::string(stamp) : port::files::gameSource());
        std::ofstream((s_dir + "/done").c_str(), std::ios::binary) << s_ids.size() << "\n";
        fprintf(stderr, "medecomp: превью кусков: %zu -> %s\n", s_ids.size(), s_dir.c_str());
        s_phase = kDone;
        finish(0);
    }
    int id = s_ids[s_index];
    std::string black = s_dir + "/.black.png", white = s_dir + "/.white.png", side = s_dir + "/.side.png";
    if (s_phase == kBlack) {
        const std::string progress = s_dir + "/progress";
        std::ofstream((progress + ".part").c_str()) << s_index << ' ' << s_ids.size() << '\n';
        remove(progress.c_str());
        rename((progress + ".part").c_str(), progress.c_str());
    }
    switch (s_phase) {
    case kBlack:
        if (!placePiece(id)) {
            s_index++;
            break;
        }
        render(g, 0xff000000);
        port::platform::requestFrameShot(black);
        s_phase = kWhite;
        break;
    case kWhite:
        render(g, 0xffffffff);
        port::platform::requestFrameShot(white);
        s_phase = kCombine;
        break;
    case kCombine:
        if (port::platform::skins::combinePreview(black, white, s_dir + "/" + std::to_string(id) + ".png"))
            shrink(s_dir + "/" + std::to_string(id) + ".png");
        else
            fprintf(stderr, "medecomp: превью куска %d не вышло\n", id);
        unlink(black.c_str());
        unlink(white.c_str());
        sideCamera(true);
        render(g, kSideColor);
        port::platform::requestFrameShot(side);
        s_phase = kLeft;
        break;
    case kLeft:
        s_left = sideCover(side);
        sideCamera(false);
        render(g, kSideColor);
        port::platform::requestFrameShot(side);
        s_phase = kRight;
        break;
    case kRight:
        s_right = sideCover(side);
        topCamera();
        render(g, kSideColor);
        port::platform::requestFrameShot(side);
        s_phase = kTop;
        break;
    case kTop:
        s_sides << id << ' ' << s_left << ' ' << s_right << ' ' << sideCover(side) << '\n';
        unlink(side.c_str());
        frontCamera();
        render(g, 0xff000000);
        port::platform::requestFrameShot(black);
        s_phase = kFrontBlack;
        break;
    case kFrontBlack:
        render(g, 0xffffffff);
        port::platform::requestFrameShot(white);
        s_phase = kFrontWhite;
        break;
    case kFrontWhite: {
        const std::string front = s_dir + "/" + std::to_string(id) + ".front.png";
        if (port::platform::skins::combinePreview(black, white, front)) shrink(front);
        unlink(black.c_str());
        unlink(white.c_str());
        const port::platform::PiecePlanes& p = s_planes;
        s_surface << id << ' ' << p.front << ' ' << p.roof << ' ' << p.left << ' ' << p.right << ' ' << int(p.hasFront) << ' '
                  << int(p.hasRoof) << ' ' << int(p.hasLeft) << ' ' << int(p.hasRight) << ' ' << p.faceLo[0] << ' ' << p.faceLo[1] << ' '
                  << p.faceHi[0] << ' ' << p.faceHi[1] << '\n';
        viewCamera();
        render(g, 0xff000000);
        port::platform::requestFrameShot(black);
        s_phase = kViewBlack;
        break;
    }
    case kViewBlack:
        render(g, 0xffffffff);
        port::platform::requestFrameShot(white);
        s_phase = kViewWhite;
        break;
    default: {
        const std::string view = s_dir + "/" + std::to_string(id) + ".view.png";
        if (port::platform::skins::combinePreview(black, white, view)) {
            shrink(view);
            s_views << id << ' ' << opaqueShare(view) << '\n';
        }
        unlink(black.c_str());
        unlink(white.c_str());
        render(g, kSideColor);
        s_index++;
        s_phase = kBlack;
        break;
    }
    }
    return true;
}

bool mapPreview(midp::Graphics* g, const char* out) {
    static int settle = 0;
    static bool shot = false;
    if (shot) finish(0);
    AppEngine* app = AppEngine::getCanvas();
    SceneGame* scene = loadedLevel(&settle);
    if (!scene) return false;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    const char* region = getenv("MEDECOMP_MAP_REGION");
    if (!region || sscanf(region, "%f,%f,%f,%f", &x0, &y0, &x1, &y1) != 4) {
        fprintf(stderr, "medecomp: превью карты — нет MEDECOMP_MAP_REGION\n");
        finish(1);
    }
    GameScreen* screen = scene->getGameScreen(0);
    m3g::World* world = static_cast<m3g::World*>(screen->m_world);
    m3g::Camera* gameCamera = static_cast<m3g::Camera*>(screen->m_m3gCamera);
    GameObjectPlayer* player = screen->getPlayerObject();
    if (!world || !gameCamera || !player) return false;
    float cx = (x0 + x1) / 2, cy = (y0 + y1) / 2, cz = player->m_position.z;
    float t = tanf(gameCamera->m_fovy * 0.5f * 3.14159265f / 180.0f);
    float d = std::max(fabsf(x1 - x0) / 2 / t, fabsf(y1 - y0) / 2 / (t * gameCamera->m_aspect)) * 1.1f + 2.0f;
    float from[3] = {cx, cy, cz + d};
    m3g::Camera* camera = new m3g::Camera();
    camera->setPerspective(gameCamera->m_fovy, gameCamera->m_aspect, 1.0f, d + 400.0f);
    m3g::Transform placed;
    placed.postTranslate(from[0], from[1], from[2]);
    placed.postRotate(90.0f, 0.0f, 0.0f, 1.0f);
    camera->setTransform(&placed);
    world->addChild(camera);
    world->setActiveCamera(camera);
    GameCamera* cam = screen->getGameCamera();
    MathFrustum* frustum = screen->getCameraFrustum();
    frustum->set(MathVector(from[0], from[1], from[2]), MathVector(cx, cy, cz), 1.0f, d + 400.0f,
                 medecomp_wide_fov(cam->getFOV()) * 1.2f, float(app->getWidth()), float(app->getHeight()));
    scene->getMap().cullView(*frustum);
    scene->getMap().updateObjectCulling(0);
    if (m3g::Node* node = player->getObjectNode()) node->setRenderingEnable(false);
    if (m3g::Node* dome = scene->getMap().getSkyDomeNode()) {
        dome->setTranslation(from[0], from[1], from[2]);
        medecomp_sky_follow(dome);
    }
    m3g::Graphics3D* g3d = app->getGraphics3D();
    g3d->bindTarget(g);
    g3d->setViewport(0, 0, app->getWidth(), app->getHeight());
    g3d->clear(nullptr);
    g3d->render(world);
    g3d->releaseTarget();
    port::platform::requestFrameShot(out);
    fprintf(stderr, "medecomp: превью карты (%.1f, %.1f)–(%.1f, %.1f) -> %s\n", x0, y0, x1, y1, out);
    shot = true;
    return true;
}

}

extern "C" void medecomp_level_preview_progress(int* done, int* total) {
    *done = int(s_index);
    *total = int(s_ids.size());
}

extern "C" bool medecomp_level_preview(midp::Graphics* g) {
    static const char* pieces = getenv("MEDECOMP_PIECE_PREVIEWS");
    static const char* map = getenv("MEDECOMP_MAP_PREVIEW");
    if (pieces && *pieces && s_phase != kDone) return piecePreviews(g, pieces);
    if (map && *map) return mapPreview(g, map);
    return false;
}
