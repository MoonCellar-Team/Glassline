#include "skins.h"
#include "settings.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "../gfx/png.h"
#include "../gfx/texture_pack.h"
#include "gltf.h"
#include "heroine_model.h"
#include "port/online.h"
#include "game/AppEngine.h"
#include "game/M3GAssets.h"
#include "game/M3GAssets_Model.h"
#include "game/M3GAssets_Texture.h"
#include "m3g/Appearance.h"
#include "m3g/Node.h"
#include "m3g/Image2D.h"
#include "m3g/Mesh.h"
#include "m3g/SkinnedMesh.h"
#include "m3g/Texture2D.h"
#include "m3g/m3g.h"
#include "port/files.h"
#include "port/net.h"

namespace settings = port::platform::settings;

namespace {

struct GameModel {
    const char* key;
    int model;
    int keep;
    int textureModel;
    const char* ru;
    const char* en;
};

const GameModel kGameModels[] = {
    {"game:faith-day", 7, 102, -1, "Фейт — день", "Faith — midday"},
    {"game:faith-dusk", 8, 102, -1, "Фейт — вечер", "Faith — dusk"},
    {"game:faith-night", 9, 102, -1, "Фейт — ночь", "Faith — night"},
    {"game:faith-interior", 10, 102, -1, "Фейт — в помещении", "Faith — interior"},
    {"game:faith-jail", 11, 102, -1, "Фейт — тюрьма", "Faith — jail"},
    {"game:faith-underground", 12, 102, -1, "Фейт — подземка", "Faith — underground"},
    {"game:ghost", 13, 102, -1, "Призрак", "Ghost"},
    {"game:runner", -1, 103, 15, "Бегун", "Runner"},
    {"game:boss", -1, 104, 16, "Босс", "Boss runner"},
};

const int kKeepFaith = 102;

struct Skin {
    std::string id;
    std::string name;
    const GameModel* base = nullptr;
    std::string texture;
    std::string model3d;
};

std::vector<Skin> s_skins;
bool s_scanned = false;
std::map<m3g::Image2D*, std::vector<uint8_t>> s_texturePixels;

std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
    return s.substr(i);
}

std::string readLine(const std::string& path) {
    std::ifstream in(path.c_str());
    std::string line;
    std::getline(in, line);
    return trim(line);
}

const GameModel* gameModel(const std::string& key) {
    for (const GameModel& m : kGameModels)
        if (key == m.key) return &m;
    return nullptr;
}

const GameModel* baseOf(const std::string& text) {
    if (text.empty()) return nullptr;
    if (const GameModel* m = gameModel(text)) return m;
    char* end = nullptr;
    long value = strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') return nullptr;
    if (value == 15) return gameModel("game:runner");
    if (value == 16) return gameModel("game:boss");
    for (const GameModel& m : kGameModels)
        if (m.model == value) return &m;
    fprintf(stderr, "medecomp: скин: модель %ld не годится для героини — как в уровне\n", value);
    return nullptr;
}

bool naturalLess(const std::string& a, const std::string& b) {
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (isdigit(static_cast<unsigned char>(a[i])) && isdigit(static_cast<unsigned char>(b[j]))) {
            size_t ei = i, ej = j;
            while (ei < a.size() && isdigit(static_cast<unsigned char>(a[ei]))) ei++;
            while (ej < b.size() && isdigit(static_cast<unsigned char>(b[ej]))) ej++;
            std::string x = a.substr(i, ei - i), y = b.substr(j, ej - j);
            x.erase(0, std::min(x.find_first_not_of('0'), x.size()));
            y.erase(0, std::min(y.find_first_not_of('0'), y.size()));
            if (x.size() != y.size()) return x.size() < y.size();
            if (x != y) return x < y;
            i = ei, j = ej;
            continue;
        }
        if (a[i] != b[j]) return a[i] < b[j];
        i++, j++;
    }
    return a.size() - i < b.size() - j;
}

void scan() {
    if (s_scanned) return;
    s_scanned = true;
    s_skins.clear();
    std::string root = port::platform::skins::folder();
    DIR* dir = opendir(root.c_str());
    if (!dir) return;
    while (dirent* ent = readdir(dir)) {
        if (ent->d_name[0] == '.') continue;
        std::string id = ent->d_name;
        std::string path = root + "/" + id;
        DIR* sub = opendir(path.c_str());
        if (!sub) continue;
        closedir(sub);
        Skin skin;
        skin.id = id;
        skin.name = readLine(path + "/name");
        if (skin.name.empty()) skin.name = id;
        skin.base = baseOf(readLine(path + "/model"));
        std::ifstream png((path + "/texture.png").c_str(), std::ios::binary);
        if (png) skin.texture = path + "/texture.png";
        for (const char* name : {"model.glb", "model.gltf"})
            if (skin.model3d.empty() && std::ifstream((path + "/" + name).c_str())) skin.model3d = path + "/" + name;
        s_skins.push_back(skin);
    }
    closedir(dir);
    std::sort(s_skins.begin(), s_skins.end(), [](const Skin& a, const Skin& b) { return naturalLess(a.id, b.id); });
}

const Skin* skinOf(const std::string& id) {
    scan();
    if (id.empty() || gameModel(id)) return nullptr;
    for (const Skin& skin : s_skins)
        if (skin.id == id) return &skin;
    return nullptr;
}

int applyTexture(m3g::Node* node, m3g::Image2D* image) {
    if (node == nullptr || image == nullptr) return 0;
    int replaced = 0;
    if (m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node)) {
        for (int i = 0; i < mesh->getSubmeshCount(); i++) {
            m3g::Appearance* appearance = mesh->getAppearance(i);
            m3g::Texture2D* texture = appearance ? appearance->getTexture(0) : nullptr;
            if (!texture) continue;
            m3g::Appearance* copy = M3GAssets::createTexturedAppearance(
                appearance, image, texture->getLevelFilter(), texture->getImageFilter(), texture->getBlending());
            mesh->m_mutable = true;
            mesh->setAppearance(i, copy);
            replaced++;
        }
        if (m3g::SkinnedMesh* skinned = m3g::m3g_cast<m3g::SkinnedMesh>(node))
            replaced += applyTexture(skinned->getSkeleton(), image);
        return replaced;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (!group) return 0;
    for (int i = 0; i < group->getChildCount(); i++) replaced += applyTexture(group->getChild(i), image);
    return replaced;
}

}

namespace port {
namespace platform {
namespace skins {

m3g::Image2D* firstTextureImage(m3g::Node* node) {
    if (node == nullptr) return nullptr;
    if (m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node)) {
        for (int i = 0; i < mesh->getSubmeshCount(); i++) {
            m3g::Appearance* appearance = mesh->getAppearance(i);
            m3g::Texture2D* texture = appearance ? appearance->getTexture(0) : nullptr;
            if (texture && texture->getImage()) return texture->getImage();
        }
        return nullptr;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (!group) return nullptr;
    for (int i = 0; i < group->getChildCount(); i++)
        if (m3g::Image2D* image = firstTextureImage(group->getChild(i))) return image;
    return nullptr;
}

m3g::Image2D* modelTexture(int model) {
    M3GAssets* assets = AppEngine::getM3GAssets();
    if (!assets || model < 0 || model >= assets->m_models.m_length) return nullptr;
    M3GAssets_Model* m = assets->m_models[model];
    if (m && m->m_texture) {
        m3g::Appearance* appearance = m->m_texture->loadCached(4);
        m3g::Texture2D* texture = appearance ? appearance->getTexture(0) : nullptr;
        if (texture && texture->getImage()) return texture->getImage();
    }
    return firstTextureImage(assets->loadModel(model, 4));
}

Look lookOf(const std::string& key) {
    Look look;
    const GameModel* m = gameModel(key);
    const Skin* skin = m ? nullptr : skinOf(key);
    if (skin) m = skin->base;
    if (m) {
        look.model = m->model;
        look.keep = m->keep;
        look.textureModel = m->textureModel;
    }
    if (skin) look.texture = skin->texture, look.model3d = skin->model3d;
    return look;
}

std::string baseKeyOf(const std::string& key) {
    if (gameModel(key)) return key;
    const Skin* skin = skinOf(key);
    return skin && skin->base ? skin->base->key : std::string();
}

bool plainFaith(const Look& look) {
    return look.keep == kKeepFaith && look.texture.empty() && look.model3d.empty() && look.model != 13;
}

void recolorShirt(m3g::Node* root, const Look& look) {
    m3g::Node* figure = root ? static_cast<m3g::Node*>(root->find(look.keep)) : nullptr;
    m3g::Image2D* image = firstTextureImage(figure);
    std::vector<uint8_t> rgba, mask;
    const char* why = "";
    if (!image) return;
    if (!port::gfx::imageRGBA(image, &rgba, &why)) {
        auto it = s_texturePixels.find(image);
        if (it == s_texturePixels.end()) {
            fprintf(stderr, "medecomp: майку не перекрасить — %s\n", why);
            return;
        }
        rgba = it->second;
    }
    int w = image->getMipMapWidth(0), h = image->getMipMapHeight(0);
    if (!port::platform::heroine::torsoMask(figure, w, h, &mask)) return;
    const float kShirt[3] = {40.0f, 110.0f, 215.0f};
    int changed = 0;
    for (size_t i = 0; i < size_t(w) * size_t(h); i++) {
        if (!mask[i]) continue;
        uint8_t* p = &rgba[i * 4];
        float luma = 0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2];
        if (luma > 90.0f) continue;
        float k = 0.35f + luma / 90.0f * 0.9f;
        for (int c = 0; c < 3; c++) p[c] = uint8_t(std::min(255.0f, kShirt[c] * k));
        changed++;
    }
    m3g::Image2D* recolored = new m3g::Image2D();
    recolored->set(100, w, h);
    recolored->commit(rgba.data());
    applyTexture(figure, recolored);
    fprintf(stderr, "medecomp: героиня — майка гостя синяя (%d точек)\n", changed);
}

void dress(m3g::Node* root, const Look& look) {
    if (!root) return;
    M3GAssets* assets = AppEngine::getM3GAssets();
    int keep = look.keep;
    m3g::Node* figure = static_cast<m3g::Node*>(root->find(keep));
    if (!figure) {
        keep = kKeepFaith;
        figure = static_cast<m3g::Node*>(root->find(keep));
    }
    if (m3g::Image2D* image = firstTextureImage(figure)) {
        std::vector<uint8_t> rgba;
        const char* why = "";
        if (!s_texturePixels.count(image) && port::gfx::imageRGBA(image, &rgba, &why)) s_texturePixels[image] = rgba;
    }
    m3g::Node* target = figure ? figure : root;
    if (look.textureModel >= 0 && assets && look.textureModel < assets->m_models.m_length) {
        int replaced = applyTexture(target, modelTexture(look.textureModel));
        fprintf(stderr, "medecomp: героиня — фигура %d, текстура модели %d (%d)\n", keep, look.textureModel, replaced);
    }
    if (!look.model3d.empty() && figure) {
        gltf::Model model;
        std::string error;
        if (!gltf::read(look.model3d, &model, &error)) {
            fprintf(stderr, "medecomp: своя модель %s — %s\n", look.model3d.c_str(), error.c_str());
        } else {
            if (port::platform::heroine::wearModel(figure, model, nullptr, &error)) {
                port::platform::heroine::hideOthers(root, figure);
                fprintf(stderr, "medecomp: своя модель %s — %zu вершин, %zu треугольников, частей %zu\n", look.model3d.c_str(),
                        model.vertexCount(), model.indices.size() / 3, model.parts.size());
            } else {
                fprintf(stderr, "medecomp: своя модель %s — %s\n", look.model3d.c_str(), error.c_str());
            }
        }
    }
    if (!look.texture.empty()) {
        std::ifstream in(look.texture.c_str(), std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        port::gfx::Image png;
        if (bytes.empty() || !port::gfx::decodePNG(bytes.data(), bytes.size(), png, false) || png.rgba.empty()) {
            fprintf(stderr, "medecomp: не прочитать текстуру скина %s\n", look.texture.c_str());
        } else {
            m3g::Image2D* image = new m3g::Image2D();
            image->set(100, png.width, png.height);
            image->commit(png.rgba.data());
            int replaced = applyTexture(target, image);
            fprintf(stderr, "medecomp: текстура скина %s (%dx%d), заменено %d\n", look.texture.c_str(), png.width,
                    png.height, replaced);
        }
    }
    for (int id = 102; id <= 104; id++)
        if (id != keep) M3GAssets::orphanNode(static_cast<m3g::Node*>(root->find(id)));
    if (!root->find(105) || !root->find(106) || !root->find(110))
        fprintf(stderr, "medecomp: героиня — в модели %d нет узлов эффектов 105, 106, 110\n", look.model);
}

}
}
}

namespace port {
namespace platform {
namespace skins {

std::string folder() { return port::files::dataDir() + "/skins"; }

std::string previewFile(const Choice& c) {
    std::string name = c.key.empty() ? "level" : c.custom ? "skin-" + c.key : c.key;
    for (char& ch : name)
        if (!(isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_')) ch = '-';
    return name + ".png";
}

std::string previewStamp(const std::string& game) {
    std::string out = game + "\n";
    for (const Choice& c : list()) {
        out += c.key;
        struct stat st;
        if (c.custom && stat((folder() + "/" + c.key + "/texture.png").c_str(), &st) == 0)
            out += " " + std::to_string(long(st.st_mtime));
        out += "\n";
    }
    return out;
}

void rescan() { s_scanned = false; }

std::vector<Choice> list() {
    scan();
    std::vector<Choice> out;
    out.push_back({"", "Как в уровне", "Level default", false});
    for (const GameModel& m : kGameModels) out.push_back({m.key, m.ru, m.en, false});
    for (const Skin& skin : s_skins) out.push_back({skin.id, skin.name, skin.name, true});
    return out;
}

std::string selected() {
    std::string id = settings::get("skin", "");
    if (id.empty() || gameModel(id) || skinOf(id)) return id;
    return "";
}

void select(const std::string& key) {
    settings::set("skin", key);
    if (!settings::save()) fprintf(stderr, "medecomp: не записать %s\n", settings::path().c_str());
}

}
}
}

extern "C" int medecomp_player_model(int levelModel) {
    port::platform::skins::Look look = port::platform::skins::lookOf(settings::get("skin", ""));
    return look.model < 0 ? levelModel : look.model;
}

extern "C" void medecomp_dress_player(m3g::Node* root) {
    s_texturePixels.clear();
    port::platform::skins::Look look = port::platform::skins::lookOf(settings::get("skin", ""));
    port::platform::skins::dress(root, look);
    if (port::net::shirtForGuest(look, true)) port::platform::skins::recolorShirt(root, look);
}
