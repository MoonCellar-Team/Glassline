#include "skins.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "../gfx/png.h"
#include "gltf.h"
#include "heroine_model.h"
#include "../gfx/texture_pack.h"
#include "game/AnimationBlender.h"
#include "game/AnimationBlenderData.h"
#include "game/AppEngine.h"
#include "game/GameObjectRunnerData.h"
#include "game/M3GAssets.h"
#include "m3g/Background.h"
#include "m3g/Camera.h"
#include "m3g/Graphics3D.h"
#include "m3g/Image2D.h"
#include "m3g/Node.h"
#include "m3g/Transform.h"
#include "m3g/World.h"
#include "port/files.h"
#include "port/platform.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/ReferenceCounted.h"

namespace skins = port::platform::skins;

namespace {

const int kSceneMenu = 2;
const int kSettleFrames = 30;
const int kModelFallback = 7;
const float kFov = 30.0f;
const float kCameraY = 0.95f, kCameraZ = 5.0f;
const float kTurn = -35.0f;
const int kPad = 6;

enum Phase { kWait, kBlack, kWhite, kCombine, kDone };

Phase s_phase = kWait;
int s_settle = 0;
size_t s_index = 0;
std::vector<skins::Choice> s_choices;
std::string s_dir;
midp::ReferenceCountedPointer<m3g::World> s_world(NULL);
midp::ReferenceCountedPointer<m3g::Group> s_holder(NULL);
midp::ReferenceCountedPointer<m3g::Background> s_background(NULL);
std::vector<AnimationBlender*> s_blenders;

m3g::Node* buildFigure(const std::string& key, m3g::Group* holder) {
    M3GAssets* assets = AppEngine::getM3GAssets();
    skins::Look look = skins::lookOf(key);
    int model = look.model < 0 ? kModelFallback : look.model;
    m3g::Node* loaded = assets ? assets->loadModel(model, 4) : nullptr;
    if (!loaded) return nullptr;
    m3g::Node* copy = static_cast<m3g::Node*>(loaded->duplicate());
    m3g::Group* root = new m3g::Group();
    holder->addChild(root);
    root->addChild(copy);
    skins::dress(root, look);
    if (GameObjectRunnerData* data = AppEngine::getGameObjectRunnerData()) {
        const GameObjectData_SoloVisual& stand = data->getSoloAnim(0);
        AnimationBlender* blender = new AnimationBlender(0);
        blender->setNode(root);
        blender->setAllChannelWeightsToZero(true);
        blender->setChannelWeight(stand.channel, 1.0f, stand.blendFlags | 3);
        blender->update(2000, true);
        s_blenders.push_back(blender);
    }
    return root;
}

void setupWorld() {
    AppEngine* app = AppEngine::getCanvas();
    AppEngine::getAnimationBlenderData()->loadData();
    AppEngine::getGameObjectRunnerData()->loadData();
    s_world = new m3g::World();
    m3g::Camera* camera = new m3g::Camera();
    camera->setPerspective(medecomp_wide_fov(kFov), medecomp_wide_aspect(float(app->getHeight())), 0.1f, 50.0f);
    m3g::Transform t;
    t.postTranslate(0.0f, kCameraY, kCameraZ);
    t.postRotate(90.0f, 0.0f, 0.0f, 1.0f);
    camera->setTransform(&t);
    s_world->addChild(camera);
    s_world->setActiveCamera(camera);
    s_background = new m3g::Background();
    s_background->setColorClearEnable(true);
    s_background->setDepthClearEnable(true);
    s_world->setBackground(s_background);
    s_holder = new m3g::Group();
    s_holder->setOrientation(kTurn, 0.0f, 1.0f, 0.0f);
    s_world->addChild(s_holder);
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

bool readShot(const std::string& path, port::gfx::Image* out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return !bytes.empty() && port::gfx::decodePNG(bytes.data(), bytes.size(), *out, false);
}

}

bool skins::combinePreview(const std::string& black, const std::string& white, const std::string& out) {
    port::gfx::Image b, w;
    if (!readShot(black, &b) || !readShot(white, &w) || b.width != w.width || b.height != w.height) return false;
    int n = b.width * b.height;
    std::vector<uint8_t> rgba(size_t(n) * 4);
    int x0 = b.width, y0 = b.height, x1 = -1, y1 = -1;
    for (int i = 0; i < n; i++) {
        const uint8_t* pb = &b.rgba[size_t(i) * 4];
        const uint8_t* pw = &w.rgba[size_t(i) * 4];
        float diff = ((pw[0] - pb[0]) + (pw[1] - pb[1]) + (pw[2] - pb[2])) / (3.0f * 255.0f);
        float a = std::min(1.0f, std::max(0.0f, 1.0f - diff));
        uint8_t* d = &rgba[size_t(i) * 4];
        for (int k = 0; k < 3; k++) d[k] = a > 0.0f ? uint8_t(std::min(255.0f, pb[k] / a)) : 0;
        d[3] = uint8_t(a * 255.0f + 0.5f);
        if (d[3] > 8) {
            int x = i % b.width, y = i / b.width;
            x0 = std::min(x0, x), x1 = std::max(x1, x), y0 = std::min(y0, y), y1 = std::max(y1, y);
        }
    }
    if (x1 < 0) return false;
    x0 = std::max(0, x0 - kPad), y0 = std::max(0, y0 - kPad);
    x1 = std::min(b.width - 1, x1 + kPad), y1 = std::min(b.height - 1, y1 + kPad);
    int cw = x1 - x0 + 1, ch = y1 - y0 + 1;
    std::vector<uint8_t> crop(size_t(cw) * ch * 4);
    for (int y = 0; y < ch; y++)
        std::copy(&rgba[(size_t(y0 + y) * b.width + x0) * 4], &rgba[(size_t(y0 + y) * b.width + x0 + cw) * 4],
                  &crop[size_t(y) * cw * 4]);
    return port::gfx::writePNG(out, cw, ch, crop.data());
}

namespace {

void exportTemplate(const char* path) {
    M3GAssets* assets = AppEngine::getM3GAssets();
    m3g::Node* loaded = assets ? assets->loadModel(kModelFallback, 4) : nullptr;
    if (!loaded) return;
    m3g::Node* copy = static_cast<m3g::Node*>(loaded->duplicate());
    m3g::Group* root = new m3g::Group();
    midp::INCREF(root);
    root->addChild(copy);
    skins::dress(root, skins::lookOf("game:faith-day"));
    m3g::Node* figure = static_cast<m3g::Node*>(root->find(102));
    std::string png;
    std::vector<uint8_t> rgba;
    const char* why = "";
    m3g::Image2D* image = skins::firstTextureImage(figure);
    std::string tmp = std::string(path) + ".png";
    if (image && port::gfx::imageRGBA(image, &rgba, &why) &&
        port::gfx::writePNG(tmp, image->getMipMapWidth(0), image->getMipMapHeight(0), rgba.data())) {
        std::ifstream in(tmp.c_str(), std::ios::binary);
        png.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        unlink(tmp.c_str());
    }
    port::platform::gltf::Model model;
    std::string error;
    if (!port::platform::heroine::faithTemplate(figure, png, &model, &error) ||
        !port::platform::gltf::write(path, model, &error)) {
        fprintf(stderr, "medecomp: шаблон — %s\n", error.c_str());
        return;
    }
    fprintf(stderr, "medecomp: шаблон %s — %zu вершин, %zu костей\n", path, model.vertexCount(), model.bones.size());
    if (getenv("MEDECOMP_SKIN_BONES"))
        for (size_t b = 0; b < model.bones.size(); b++) {
            const float* m = &model.inverseBind[b * 16];
            float x = -(m[0] * m[12] + m[1] * m[13] + m[2] * m[14]);
            float y = -(m[4] * m[12] + m[5] * m[13] + m[6] * m[14]);
            float z = -(m[8] * m[12] + m[9] * m[13] + m[10] * m[14]);
            fprintf(stderr, "medecomp: кость %2zu %-12s родитель %2d  (%.3f, %.3f, %.3f)\n", b, model.bones[b].name.c_str(),
                    model.bones[b].parent, x, y, z);
        }
}

void exportTexture(const char* spec) {
    std::string s = spec;
    size_t bar = s.find('|');
    if (bar == std::string::npos) return;
    std::string key = s.substr(0, bar), path = s.substr(bar + 1);
    skins::Look look = skins::lookOf(key);
    M3GAssets* assets = AppEngine::getM3GAssets();
    int model = look.model < 0 ? kModelFallback : look.model;
    m3g::Image2D* image = nullptr;
    if (look.textureModel >= 0) {
        image = skins::modelTexture(look.textureModel);
    } else if (m3g::Node* node = assets->loadModel(model, 4)) {
        image = skins::firstTextureImage(static_cast<m3g::Node*>(node->find(look.keep)));
    }
    std::vector<uint8_t> rgba;
    const char* why = "нет текстуры";
    if (!image || !port::gfx::imageRGBA(image, &rgba, &why)) {
        fprintf(stderr, "medecomp: текстура «%s» — %s\n", key.c_str(), why);
        return;
    }
    int w = image->getMipMapWidth(0), h = image->getMipMapHeight(0);
    bool ok = port::gfx::writePNG(path, w, h, rgba.data());
    fprintf(stderr, "medecomp: текстура «%s» %dx%d -> %s%s\n", key.c_str(), w, h, path.c_str(), ok ? "" : " — не записать");
#ifdef __ANDROID__
    size_t slash = path.find_last_of('/'), prev = slash == std::string::npos ? slash : path.find_last_of('/', slash - 1);
    if (ok && prev != std::string::npos) {
        std::string id = path.substr(prev + 1, slash - prev - 1);
        std::string shown = port::platform::androidSaveToDownloads(path, id + "-texture.png", "image/png");
        if (!shown.empty()) std::ofstream((s_dir + "/exported-" + id).c_str()) << shown;
    }
#endif
}

}

extern "C" void medecomp_skin_preview_progress(int* done, int* total) {
    *done = int(s_index);
    *total = int(s_choices.size());
}

extern "C" bool medecomp_skin_preview(midp::Graphics* g) {
    static const char* dir = getenv("MEDECOMP_SKIN_PREVIEWS");
    if (!dir || !*dir || s_phase == kDone) return false;
    AppEngine* app = AppEngine::getCanvas();
    if (s_phase == kWait) {
        if (!app || app->m_sceneId != kSceneMenu || !AppEngine::getM3GAssets()) return false;
        if (++s_settle < kSettleFrames) return false;
        s_dir = dir;
        s_choices = skins::list();
        if (const char* spec = getenv("MEDECOMP_SKIN_EXPORT")) exportTexture(spec);
        if (const char* path = getenv("MEDECOMP_SKIN_TEMPLATE")) exportTemplate(path);
        setupWorld();
        s_phase = kBlack;
    }
    if (s_index >= s_choices.size()) {
        std::ofstream((s_dir + "/stamp").c_str()) << skins::previewStamp(port::files::gameSource());
        std::ofstream((s_dir + "/done").c_str()) << s_choices.size() << "\n";
        fprintf(stderr, "medecomp: превью героини: %zu -> %s\n", s_choices.size(), s_dir.c_str());
        s_phase = kDone;
#ifdef __ANDROID__
        port::platform::androidRestart();
#endif
        _exit(0);
    }
    const skins::Choice& choice = s_choices[s_index];
    std::string black = s_dir + "/.black.png", white = s_dir + "/.white.png";
    switch (s_phase) {
    case kBlack:
        while (s_holder->getChildCount() > 0) s_holder->removeChild(s_holder->getChild(0));
        buildFigure(choice.key, s_holder);
        render(g, 0xff000000);
        port::platform::requestFrameShot(black);
        s_phase = kWhite;
        break;
    case kWhite:
        render(g, 0xffffffff);
        port::platform::requestFrameShot(white);
        s_phase = kCombine;
        break;
    default: {
        render(g, 0xffffffff);
        std::string out = s_dir + "/" + skins::previewFile(choice);
        if (!skins::combinePreview(black, white, out)) fprintf(stderr, "medecomp: превью «%s» не вышло\n", choice.key.c_str());
        unlink(black.c_str());
        unlink(white.c_str());
        s_index++;
        s_phase = kBlack;
        break;
    }
    }
    return true;
}
