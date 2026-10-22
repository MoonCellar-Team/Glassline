#include "autorig.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <memory>
#include <set>

#include "../gfx/png.h"
#include "port/online.h"

namespace port {
namespace platform {
namespace autorig {

using gltf::T;

namespace {

struct V3 {
    float x = 0, y = 0, z = 0;
    V3() {}
    V3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    V3 operator+(const V3& o) const { return V3(x + o.x, y + o.y, z + o.z); }
    V3 operator-(const V3& o) const { return V3(x - o.x, y - o.y, z - o.z); }
    V3 operator*(float k) const { return V3(x * k, y * k, z * k); }
    float operator[](int i) const { return i == 0 ? x : i == 1 ? y : z; }
};

float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 cross(V3 a, V3 b) { return V3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
float length(V3 a) { return sqrtf(dot(a, a)); }
V3 normalize(V3 a) {
    float l = length(a);
    return l > 1e-12f ? a * (1.0f / l) : V3();
}

struct M3 {
    float m[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    V3 operator*(V3 v) const {
        return V3(m[0] * v.x + m[1] * v.y + m[2] * v.z, m[3] * v.x + m[4] * v.y + m[5] * v.z, m[6] * v.x + m[7] * v.y + m[8] * v.z);
    }
    M3 operator*(const M3& o) const {
        M3 r;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                float s = 0;
                for (int k = 0; k < 3; k++) s += m[i * 3 + k] * o.m[k * 3 + j];
                r.m[i * 3 + j] = s;
            }
        return r;
    }
    M3 transposed() const {
        M3 r;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) r.m[i * 3 + j] = m[j * 3 + i];
        return r;
    }
};

M3 rotation(V3 a, V3 b) {
    a = normalize(a), b = normalize(b);
    V3 v = cross(a, b);
    float c = dot(a, b), s = length(v);
    M3 r;
    if (s < 1e-6f) {
        if (c > 0) return r;
        V3 axis = normalize(fabsf(a.x) < 0.9f ? cross(a, V3(1, 0, 0)) : cross(a, V3(0, 1, 0)));
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) r.m[i * 3 + j] = 2 * axis[i] * axis[j] - (i == j ? 1.0f : 0.0f);
        return r;
    }
    V3 k = v * (1.0f / s);
    float t = 1 - c;
    r.m[0] = c + k.x * k.x * t, r.m[1] = k.x * k.y * t - k.z * s, r.m[2] = k.x * k.z * t + k.y * s;
    r.m[3] = k.y * k.x * t + k.z * s, r.m[4] = c + k.y * k.y * t, r.m[5] = k.y * k.z * t - k.x * s;
    r.m[6] = k.z * k.x * t - k.y * s, r.m[7] = k.z * k.y * t + k.x * s, r.m[8] = c + k.z * k.z * t;
    return r;
}

M3 stretch(V3 d, float k) {
    M3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) r.m[i * 3 + j] = (i == j ? 1.0f : 0.0f) + (k - 1) * d[i] * d[j];
    return r;
}

V3 apply(const float* m, V3 p) {
    return V3(m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12], m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
              m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]);
}

void multiply(const float* a, const float* b, float* out) {
    float m[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float v = 0;
            for (int k = 0; k < 4; k++) v += a[k * 4 + r] * b[c * 4 + k];
            m[c * 4 + r] = v;
        }
    memcpy(out, m, sizeof m);
}

bool invert(const float* m, float* out) {
    float inv[16];
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (fabsf(det) < 1e-20f) return false;
    for (int i = 0; i < 16; i++) out[i] = inv[i] / det;
    return true;
}

enum Role {
    kRoot, kPelvis, kThighL, kCalfL, kFootL, kToeL, kSpine, kChest, kNeck, kHead, kEyeL, kEyeR, kHeadTop,
    kClavL, kUpperArmL, kForearmL, kHandL, kClavR, kUpperArmR, kForearmR, kHandR, kThighR, kCalfR, kFootR, kToeR,
    kRoles
};
const char* const kRoleNames[kRoles] = {
    "root", "pelvis", "thigh_l", "calf_l", "foot_l", "toe_l", "spine", "chest", "neck", "head", "eye_l", "eye_r", "head_top",
    "clavicle_l", "upperarm_l", "forearm_l", "hand_l", "clavicle_r", "upperarm_r", "forearm_r", "hand_r", "thigh_r",
    "calf_r", "foot_r", "toe_r",
};
const int kRequired[] = {kPelvis, kHead, kUpperArmL, kForearmL, kUpperArmR, kForearmR, kThighL, kCalfL, kThighR, kCalfR};

int sided(int left, int side) {
    if (side == 0) return -1;
    if (side == 1) return left;
    switch (left) {
    case kThighL: return kThighR;
    case kCalfL: return kCalfR;
    case kFootL: return kFootR;
    case kToeL: return kToeR;
    case kEyeL: return kEyeR;
    case kClavL: return kClavR;
    case kUpperArmL: return kUpperArmR;
    case kForearmL: return kForearmR;
    case kHandL: return kHandR;
    }
    return -1;
}

std::vector<std::string> words(const std::string& name) {
    std::string n = name.substr(name.find_last_of(':') == std::string::npos ? 0 : name.find_last_of(':') + 1);
    std::vector<std::string> out;
    std::string cur;
    auto flush = [&] {
        if (!cur.empty()) out.push_back(cur), cur.clear();
    };
    for (size_t i = 0; i < n.size(); i++) {
        unsigned char c = static_cast<unsigned char>(n[i]);
        if (!isalnum(c)) {
            flush();
            continue;
        }
        if (!cur.empty()) {
            unsigned char p = static_cast<unsigned char>(n[i - 1]);
            bool next = i + 1 < n.size() && islower(static_cast<unsigned char>(n[i + 1]));
            if ((islower(p) && isupper(c)) || (!isdigit(p) != !isdigit(c)) || (isupper(p) && isupper(c) && next)) flush();
        }
        cur += char(tolower(c));
    }
    flush();
    return out;
}

int classify(const std::string& name) {
    static const std::set<std::string> skip = {
        "thumb", "index", "middle", "ring", "pinky", "pinkie", "little", "finger", "fingers", "digit", "twist", "roll", "aim",
        "sec", "bust", "breast", "boob", "hair", "skirt", "cloth", "end", "nub", "pole", "ik", "fk", "target", "helper",
        "adj", "jaw", "tongue", "teeth", "tooth", "eyelid", "lid", "brow", "eyebrow", "lip", "lips", "cheek", "nose", "ear",
        "mch", "org", "tail", "wing", "weapon", "prop", "ctrl", "control", "ponytail", "tip", "tops", "cape", "coat", "hood",
        "bag", "attach", "socket", "dummy", "null", "slot", "corrective", "fix", "metacarpal", "carpal", "pupil", "iris",
        "look", "lookat", "camera", "light", "share", "bend", "upperlid", "lowerlid", "mouth", "face", "spring",
    };
    static const std::set<std::string> noise = {
        "mixamorig", "j", "bip", "c", "def", "cc", "base", "b", "bn", "jnt", "joint", "bone", "skeleton", "sk", "valve",
        "biped", "rig", "armature", "deform", "m", "ue", "ch", "char", "character", "bind",
    };
    std::vector<std::string> w = words(name);
    bool head = false, top = false;
    for (const std::string& s : w) head |= s == "head", top |= s == "top";
    int side = 0;
    std::string core;
    for (const std::string& s : w) {
        if (s == "l" || s == "left") side = side ? -1 : 1;
        else if (s == "r" || s == "right") side = side ? -1 : 2;
        else if (head && top) continue;
        else if (skip.count(s)) return -1;
        else if (s == "root") return -1;
        else if (!noise.count(s) && !isdigit(static_cast<unsigned char>(s[0]))) core += s;
    }
    if (side < 0) return -1;
    if (head && top) return side ? -1 : kHeadTop;
    static const std::map<std::string, int> central = {
        {"hips", kPelvis}, {"hip", kPelvis}, {"pelvis", kPelvis},
        {"spine", kSpine}, {"abdomen", kSpine}, {"belly", kSpine}, {"waist", kSpine}, {"lowerspine", kSpine},
        {"spinelower", kSpine}, {"torso", kSpine}, {"lowerback", kSpine}, {"stomach", kSpine},
        {"chest", kChest}, {"upperchest", kChest}, {"ribcage", kChest}, {"thorax", kChest}, {"upperspine", kChest},
        {"spineupper", kChest},
        {"neck", kNeck}, {"upperneck", kNeck}, {"lowerneck", kNeck},
        {"head", kHead},
    };
    static const std::map<std::string, int> limbs = {
        {"eye", kEyeL},
        {"shoulder", kClavL}, {"clavicle", kClavL}, {"collar", kClavL}, {"collarbone", kClavL}, {"clav", kClavL},
        {"upperarm", kUpperArmL}, {"uparm", kUpperArmL}, {"arm", kUpperArmL}, {"bicep", kUpperArmL}, {"humerus", kUpperArmL},
        {"armupper", kUpperArmL},
        {"forearm", kForearmL}, {"lowerarm", kForearmL}, {"elbow", kForearmL}, {"lowarm", kForearmL}, {"armlower", kForearmL},
        {"hand", kHandL}, {"wrist", kHandL},
        {"upleg", kThighL}, {"upperleg", kThighL}, {"thigh", kThighL}, {"thig", kThighL}, {"legupper", kThighL}, {"femur", kThighL},
        {"leg", kCalfL}, {"lowerleg", kCalfL}, {"calf", kCalfL}, {"shin", kCalfL}, {"knee", kCalfL}, {"lowleg", kCalfL},
        {"leglower", kCalfL},
        {"foot", kFootL}, {"ankle", kFootL},
        {"toe", kToeL}, {"toes", kToeL}, {"toebase", kToeL}, {"ball", kToeL},
    };
    auto c = central.find(core);
    if (c != central.end()) return side ? -1 : c->second;
    auto l = limbs.find(core);
    if (l != limbs.end()) return sided(l->second, side);
    return -1;
}

std::string rigName(const gltf::Scene& scene) {
    if (!scene.humanoid.empty()) return "VRM";
    int mixamo = 0, vroid = 0, cc = 0, ue = 0, rigify = 0, blender = 0;
    for (const gltf::Node& n : scene.nodes) {
        const std::string& s = n.name;
        if (s.compare(0, 9, "mixamorig") == 0) mixamo++;
        if (s.compare(0, 6, "J_Bip_") == 0) vroid++;
        if (s.compare(0, 8, "CC_Base_") == 0) cc++;
        if (s == "upperarm_l" || s == "spine_01" || s == "lowerarm_l") ue++;
        if (s.compare(0, 4, "DEF-") == 0) rigify++;
        if (s.size() > 2 && (s.compare(s.size() - 2, 2, ".L") == 0 || s.compare(s.size() - 2, 2, ".R") == 0)) blender++;
    }
    if (mixamo) return "Mixamo";
    if (vroid) return "VRoid";
    if (cc) return "Character Creator";
    if (ue >= 2) return "Unreal";
    if (rigify) return "Rigify";
    if (blender) return "Blender";
    return std::string();
}

struct Faith {
    int bone[kRoles];
    V3 joint[kRoles];
    std::vector<V3> pos;
    std::vector<std::pair<int, float>> weights;
    std::vector<uint32_t> tris;
    float top = 0;
    float heel = 0, front = 0;
    float crotch = 0;
    float reach = 0;
};

std::vector<V3> surface(const std::vector<V3>& p, const std::vector<uint32_t>& tris, float step);
float crotchHeight(const std::vector<V3>& s, float xLimit, float top);

bool loadFaith(const gltf::Model& m, Faith* f, std::string* error) {
    std::vector<int> role(m.bones.size(), -1);
    for (int r = 0; r < kRoles; r++) {
        f->bone[r] = -1;
        for (size_t b = 0; b < m.bones.size(); b++)
            if (m.bones[b].name == kRoleNames[r]) f->bone[r] = int(b), role[b] = r;
        if (f->bone[r] < 0 || m.inverseBind.size() < (size_t(f->bone[r]) + 1) * 16)
            return *error = T("шаблон Фейт не тот (нет кости ", "the Faith template is wrong (no bone ") + kRoleNames[r] + ")", false;
        float bind[16];
        if (!invert(&m.inverseBind[size_t(f->bone[r]) * 16], bind)) return *error = T("шаблон Фейт не тот", "the Faith template is wrong"), false;
        f->joint[r] = V3(bind[12], bind[13], bind[14]);
    }
    size_t n = m.vertexCount();
    float heel = 1e9f, front = -1e9f, reach = 0;
    for (size_t i = 0; i < n; i++) {
        V3 p(m.positions[i * 3], m.positions[i * 3 + 1], m.positions[i * 3 + 2]);
        f->pos.push_back(p);
        f->top = std::max(f->top, p.y);
        if (p.y < 0.03f && p.x > 0) heel = std::min(heel, p.z), front = std::max(front, p.z);
        if (p.x > f->joint[kHandL].x) reach = std::max(reach, length(p - f->joint[kHandL]));
        for (int k = 0; k < 4; k++) {
            int b = m.joints[i * 4 + size_t(k)];
            int r = b < int(role.size()) ? role[size_t(b)] : -1;
            f->weights.push_back({r < 0 ? kPelvis : r, m.weights[i * 4 + size_t(k)]});
        }
    }
    f->tris = m.indices;
    f->heel = heel, f->front = front, f->reach = reach;
    f->crotch = crotchHeight(surface(f->pos, f->tris, 0.02f), 0.3f, f->top);
    if (f->crotch <= 0) f->crotch = 0.9f;
    if (n == 0 || f->top <= 0 || heel >= front) return *error = T("шаблон Фейт не тот", "the Faith template is wrong"), false;
    return true;
}

struct Source {
    std::vector<V3> pos;
    std::vector<float> uv;
    std::vector<std::vector<std::pair<int, float>>> influence;
    std::vector<uint32_t> tris;
    std::vector<int> triPart;
    std::vector<int> partMaterial;
};

void gather(const gltf::Scene& scene, Source* src) {
    std::map<int, int> partOf;
    std::map<std::string, uint32_t> shared;
    std::vector<std::vector<float>> skinning(scene.skins.size());
    for (size_t s = 0; s < scene.skins.size(); s++) {
        const gltf::Skin& skin = scene.skins[s];
        skinning[s].resize(skin.joints.size() * 16);
        for (size_t j = 0; j < skin.joints.size(); j++)
            multiply(scene.nodes[size_t(skin.joints[j])].world, &skin.inverseBind[j * 16], &skinning[s][j * 16]);
    }
    for (size_t ni = 0; ni < scene.nodes.size(); ni++) {
        const gltf::Node& node = scene.nodes[ni];
        if (node.mesh < 0 || size_t(node.mesh) >= scene.meshes.size()) continue;
        int skinIndex = node.skin >= 0 && size_t(node.skin) < scene.skins.size() ? node.skin : -1;
        for (const gltf::Primitive& prim : scene.meshes[size_t(node.mesh)].primitives) {
            size_t count = prim.positions.size() / 3;
            uint32_t base = uint32_t(src->pos.size());
            std::string key = std::to_string(ni) + ":" + prim.vertices;
            if (shared.count(key)) base = shared[key], count = 0;
            else shared[key] = base;
            for (size_t v = 0; v < count; v++) {
                V3 raw(prim.positions[v * 3], prim.positions[v * 3 + 1], prim.positions[v * 3 + 2]);
                std::vector<std::pair<int, float>> infl;
                V3 p;
                float sum = 0;
                if (skinIndex >= 0 && !prim.joints.empty()) {
                    const gltf::Skin& skin = scene.skins[size_t(skinIndex)];
                    for (int c = 0; c < 8; c++) {
                        float w = prim.weights[v * 8 + size_t(c)];
                        size_t j = prim.joints[v * 8 + size_t(c)];
                        if (w <= 0 || j >= skin.joints.size()) continue;
                        p = p + apply(&skinning[size_t(skinIndex)][j * 16], raw) * w;
                        sum += w;
                        infl.push_back({skin.joints[j], w});
                    }
                }
                if (sum > 0) {
                    p = p * (1.0f / sum);
                    for (auto& i : infl) i.second /= sum;
                } else {
                    p = apply(node.world, raw);
                    infl.assign(1, {int(ni), 1.0f});
                }
                src->pos.push_back(p);
                src->uv.push_back(prim.uvs[v * 2]);
                src->uv.push_back(prim.uvs[v * 2 + 1]);
                src->influence.push_back(infl);
            }
            auto it = partOf.find(prim.material);
            if (it == partOf.end()) {
                it = partOf.insert({prim.material, int(src->partMaterial.size())}).first;
                src->partMaterial.push_back(prim.material);
            }
            for (size_t t = 0; t + 2 < prim.indices.size(); t += 3) {
                for (int k = 0; k < 3; k++) src->tris.push_back(base + prim.indices[t + size_t(k)]);
                src->triPart.push_back(it->second);
            }
        }
    }
}

struct Mapping {
    std::vector<int> role;
    int primary[kRoles];
};

int depthOf(const gltf::Scene& s, int n) {
    int d = 0;
    for (int k = s.nodes[size_t(n)].parent; k >= 0 && d < 1000; k = s.nodes[size_t(k)].parent) d++;
    return d;
}

bool ancestor(const gltf::Scene& s, int a, int n) {
    for (int k = n, guard = 0; k >= 0 && guard < 1000; k = s.nodes[size_t(k)].parent, guard++)
        if (k == a) return true;
    return false;
}

int commonAncestor(const gltf::Scene& s, int a, int b) {
    for (int k = a, guard = 0; k >= 0 && guard < 1000; k = s.nodes[size_t(k)].parent, guard++)
        if (ancestor(s, k, b)) return k;
    return -1;
}

void mapBones(const gltf::Scene& scene, Mapping* map) {
    size_t n = scene.nodes.size();
    map->role.assign(n, -1);
    for (int& p : map->primary) p = -1;
    if (!scene.humanoid.empty()) {
        static const std::map<std::string, int> vrm = {
            {"hips", kPelvis}, {"spine", kSpine}, {"chest", kChest}, {"upperChest", kChest}, {"neck", kNeck},
            {"head", kHead}, {"leftEye", kEyeL}, {"rightEye", kEyeR}, {"leftShoulder", kClavL}, {"rightShoulder", kClavR},
            {"leftUpperArm", kUpperArmL}, {"rightUpperArm", kUpperArmR}, {"leftLowerArm", kForearmL},
            {"rightLowerArm", kForearmR}, {"leftHand", kHandL}, {"rightHand", kHandR}, {"leftUpperLeg", kThighL},
            {"rightUpperLeg", kThighR}, {"leftLowerLeg", kCalfL}, {"rightLowerLeg", kCalfR}, {"leftFoot", kFootL},
            {"rightFoot", kFootR}, {"leftToes", kToeL}, {"rightToes", kToeR},
        };
        for (const auto& h : scene.humanoid) {
            auto it = vrm.find(h.first);
            if (it == vrm.end()) continue;
            map->role[size_t(h.second)] = it->second;
        }
        auto upper = scene.humanoid.find("upperChest"), chest = scene.humanoid.find("chest");
        if (upper != scene.humanoid.end() && chest != scene.humanoid.end()) map->role[size_t(chest->second)] = kSpine;
    } else {
        std::vector<bool> joint(n, scene.skins.empty());
        for (const gltf::Skin& s : scene.skins)
            for (int j : s.joints) joint[size_t(j)] = true;
        for (size_t i = 0; i < n; i++) {
            if (scene.skins.empty() && scene.nodes[i].mesh >= 0) joint[i] = false;
            if (joint[i]) map->role[i] = classify(scene.nodes[i].name);
        }
    }
    for (int r = 0; r < kRoles; r++) {
        int best = -1, bestDepth = 1 << 30;
        for (size_t i = 0; i < n; i++)
            if (map->role[i] == r) {
                int d = depthOf(scene, int(i));
                if (d < bestDepth) best = int(i), bestDepth = d;
            }
        map->primary[r] = best;
    }
    int* p = map->primary;
    if (p[kThighL] >= 0 && p[kThighR] >= 0) {
        int c = commonAncestor(scene, p[kThighL], p[kThighR]);
        if (c >= 0 && c != p[kThighL] && c != p[kThighR]) {
            if (p[kPelvis] >= 0 && p[kPelvis] != c) map->role[size_t(p[kPelvis])] = -1;
            p[kPelvis] = c, map->role[size_t(c)] = kPelvis;
        }
    }
    int armL = p[kClavL] >= 0 ? p[kClavL] : p[kUpperArmL], armR = p[kClavR] >= 0 ? p[kClavR] : p[kUpperArmR];
    if (armL >= 0 && armR >= 0 && p[kPelvis] >= 0) {
        int c = commonAncestor(scene, armL, armR);
        if (c >= 0 && c != p[kPelvis] && c != p[kNeck] && c != p[kHead] && ancestor(scene, p[kPelvis], c)) {
            if (p[kChest] >= 0 && p[kChest] != c && map->role[size_t(p[kChest])] == kChest) map->role[size_t(p[kChest])] = kSpine;
            p[kChest] = c, map->role[size_t(c)] = kChest;
        }
    }
    if (p[kChest] >= 0 && p[kPelvis] >= 0 && (p[kSpine] < 0 || !ancestor(scene, p[kSpine], p[kChest]) || p[kSpine] == p[kChest])) {
        int below = -1;
        for (int k = scene.nodes[size_t(p[kChest])].parent; k >= 0 && k != p[kPelvis]; k = scene.nodes[size_t(k)].parent) below = k;
        if (below >= 0) p[kSpine] = below, map->role[size_t(below)] = kSpine;
    }
    if (p[kNeck] < 0 && p[kHead] >= 0 && p[kChest] >= 0 && ancestor(scene, p[kChest], p[kHead])) {
        int below = -1;
        for (int k = scene.nodes[size_t(p[kHead])].parent; k >= 0 && k != p[kChest]; k = scene.nodes[size_t(k)].parent) below = k;
        if (below >= 0) p[kNeck] = below, map->role[size_t(below)] = kNeck;
    }
}

V3 likeFaith(const Faith& f, const V3* M, int b, int a, int c) {
    V3 fa = f.joint[a], fc = f.joint[c], ma = M[a], mc = M[c];
    float lf = length(fc - fa), lm = length(mc - ma);
    float k = lf > 1e-6f ? lm / lf : 1.0f;
    return ma + rotation(fc - fa, mc - ma) * ((f.joint[b] - fa) * k);
}

template <class Pred>
bool slab(const std::vector<V3>& p, float y, float dy, Pred pred, V3* center, int* count = nullptr) {
    V3 sum;
    int n = 0;
    for (const V3& v : p)
        if (fabsf(v.y - y) < dy && pred(v)) sum = sum + v, n++;
    if (count) *count = n;
    if (!n) return false;
    *center = sum * (1.0f / float(n));
    return true;
}

std::vector<V3> surface(const std::vector<V3>& p, const std::vector<uint32_t>& tris, float step) {
    std::vector<V3> out = p;
    for (size_t t = 0; t + 2 < tris.size() && out.size() < 600000; t += 3) {
        V3 a = p[tris[t]], b = p[tris[t + 1]], c = p[tris[t + 2]];
        float edge = std::max({length(b - a), length(c - b), length(a - c)});
        int n = std::min(40, int(edge / step));
        for (int i = 0; i <= n; i++)
            for (int j = 0; i + j <= n; j++) {
                if ((i == 0 && j == 0) || (i == n && j == 0) || (i == 0 && j == n)) continue;
                float u = float(i) / float(std::max(n, 1)), v = float(j) / float(std::max(n, 1));
                out.push_back(a + (b - a) * u + (c - a) * v);
            }
    }
    return out;
}

float crotchHeight(const std::vector<V3>& s, float xLimit, float top) {
    float last = -1;
    int merged = 0;
    for (float y = 0.04f; y < top * 0.75f; y += 0.01f) {
        std::vector<float> xs;
        for (const V3& v : s)
            if (fabsf(v.y - y) < 0.006f && fabsf(v.x) < xLimit) xs.push_back(v.x);
        if (xs.size() < 6) continue;
        std::sort(xs.begin(), xs.end());
        bool split = false;
        for (size_t i = 1; i < xs.size(); i++)
            if (xs[i] - xs[i - 1] > 0.02f && xs[i - 1] < 0.04f && xs[i] > -0.04f) split = true;
        if (split) last = y, merged = 0;
        else if (last > 0 && ++merged >= 2) break;
    }
    return last;
}

struct Arm {
    std::vector<V3> track;
    float length = 0;
};

bool findArm(const std::vector<V3>& s, float side, float top, Arm* arm) {
    float best = 0;
    V3 tip;
    for (const V3& v : s)
        if (v.y > 0.25f * top && v.x * side > best) best = v.x * side, tip = v;
    if (best <= 0) return false;
    V3 c = tip;
    std::vector<V3> track;
    std::vector<float> extents;
    int empty = 0;
    for (float x = best - 0.01f; x > 0.02f; x -= 0.02f) {
        V3 sum;
        int n = 0;
        float lo = 1e9f, hi = -1e9f;
        for (const V3& v : s) {
            if (fabsf(v.x * side - x) > 0.012f) continue;
            float dy = v.y - c.y, dz = v.z - c.z;
            if (dy > -0.45f && dy < 0.12f && fabsf(dz) < 0.2f) lo = std::min(lo, v.y), hi = std::max(hi, v.y);
            if (dy * dy + dz * dz > 0.12f * 0.12f) continue;
            sum = sum + v, n++;
        }
        if (!n) {
            if (++empty > 2) break;
            continue;
        }
        empty = 0;
        float e = hi - lo;
        if (track.size() >= 3) {
            std::vector<float> sorted = extents;
            std::sort(sorted.begin(), sorted.end());
            if (e > std::max(0.25f, 2.0f * sorted[sorted.size() / 2])) break;
        }
        c = sum * (1.0f / float(n));
        track.push_back(c);
        extents.push_back(e);
    }
    track.insert(track.begin(), tip);
    std::reverse(track.begin(), track.end());
    float len = 0;
    for (size_t i = 1; i < track.size(); i++) len += length(track[i] - track[i - 1]);
    arm->track = track;
    arm->length = len;
    return len > 0.25f;
}

V3 along(const std::vector<V3>& track, float d) {
    for (size_t i = 1; i < track.size(); i++) {
        float l = length(track[i] - track[i - 1]);
        if (d <= l) return track[i - 1] + (track[i] - track[i - 1]) * (l > 0 ? d / l : 0.0f);
        d -= l;
    }
    return track.back();
}

bool shapeJoints(const std::vector<V3>& s, const Faith& f, V3* M, std::string* error) {
    float top = 0;
    for (const V3& v : s) top = std::max(top, v.y);
    Arm arms[2];
    for (int side = 0; side < 2; side++)
        if (!findArm(s, side ? -1.0f : 1.0f, top, &arms[side]))
            return *error = T("не нашёл руку: модель должна стоять в T- или A-позе, руки — в стороны от тела",
                              "cannot find an arm: the model must stand in a T or A pose with the arms away from the body"), false;
    V3 shoulder[2];
    for (int side = 0; side < 2; side++) {
        const std::vector<V3>& t = arms[side].track;
        V3 dir = normalize(along(t, std::min(0.15f, arms[side].length)) - t.front());
        shoulder[side] = t.front() - dir * 0.03f;
    }
    float shoulderY = (shoulder[0].y + shoulder[1].y) * 0.5f;
    float shoulderX = (fabsf(shoulder[0].x) + fabsf(shoulder[1].x)) * 0.5f;
    if (shoulderY < 0.3f * top || shoulderY > 0.97f * top || shoulderX < 0.05f)
        return *error = T("не понял, где плечи: нужна T- или A-поза", "cannot tell where the shoulders are: a T or A pose is needed"), false;
    float crotch = crotchHeight(s, shoulderX, top);
    if (crotch < 0.1f * top || crotch > shoulderY - 0.1f) crotch = shoulderY * (f.crotch / f.joint[kUpperArmL].y);
    float fShoulder = f.joint[kUpperArmL].y;
    auto height = [&](float y) {
        if (y <= f.crotch) return y * crotch / f.crotch;
        if (y <= fShoulder) return crotch + (y - f.crotch) * (shoulderY - crotch) / (fShoulder - f.crotch);
        return shoulderY + (y - fShoulder) * (top - shoulderY) / (f.top - fShoulder);
    };
    float widthK = shoulderX / fabsf(f.joint[kUpperArmL].x);
    for (int r = 0; r < kRoles; r++) M[r] = V3(f.joint[r].x * widthK, height(f.joint[r].y), f.joint[r].z);
    M[kRoot] = V3();
    float column = std::max(0.06f, shoulderX * 0.5f);
    for (int r : {kPelvis, kSpine, kChest, kNeck, kHead}) {
        std::vector<float> zs;
        for (const V3& v : s)
            if (fabsf(v.y - M[r].y) < 0.03f && fabsf(v.x) < column) zs.push_back(v.z);
        if (!zs.empty()) {
            std::nth_element(zs.begin(), zs.begin() + long(zs.size() / 2), zs.end());
            M[r].z = zs[zs.size() / 2];
        }
        M[r].x = 0;
    }
    M[kHeadTop].z = M[kHead].z;
    for (int r : {kEyeL, kEyeR}) M[r].z = M[kHead].z + (f.joint[r].z - f.joint[kHead].z);
    for (int r : {kClavL, kClavR}) M[r].z = M[kChest].z + (f.joint[r].z - f.joint[kChest].z);
    for (int side = 0; side < 2; side++) {
        float sg = side ? -1.0f : 1.0f;
        int thigh = side ? kThighR : kThighL, calf = side ? kCalfR : kCalfL, foot = side ? kFootR : kFootL, toe = side ? kToeR : kToeL;
        int upper = side ? kUpperArmR : kUpperArmL, fore = side ? kForearmR : kForearmL, hand = side ? kHandR : kHandL;
        auto leg = [sg, shoulderX](const V3& v) { return v.x * sg > 0.01f && fabsf(v.x) < shoulderX; };
        V3 c;
        if (slab(s, std::max(0.05f, crotch - 0.04f), 0.02f, leg, &c)) M[thigh].x = c.x;
        M[thigh].z = M[kPelvis].z;
        for (int r : {calf, foot})
            if (slab(s, M[r].y, 0.03f, leg, &c)) M[r].x = c.x, M[r].z = c.z;
        float heel = 1e9f, front = -1e9f;
        for (const V3& v : s)
            if (v.y < 0.03f && leg(v)) heel = std::min(heel, v.z), front = std::max(front, v.z);
        if (heel < front)
            M[toe] = V3(M[foot].x, M[toe].y, heel + (front - heel) * (f.joint[toe].z - f.heel) / (f.front - f.heel));
        else
            M[toe] = M[foot] + (f.joint[toe] - f.joint[foot]);
        const Arm& arm = arms[side];
        float l1 = length(f.joint[fore] - f.joint[upper]), l2 = length(f.joint[hand] - f.joint[fore]);
        float k = (arm.length + 0.03f) / (l1 + l2 + f.reach);
        M[upper] = shoulder[side];
        M[fore] = along(arm.track, std::max(0.0f, l1 * k - 0.03f));
        M[hand] = along(arm.track, std::max(0.0f, (l1 + l2) * k - 0.03f));
    }
    return true;
}

struct Bone {
    V3 m, f;
    M3 a, inv;
};

void boneTransforms(const Faith& f, const V3* M, Bone* out) {
    auto make = [&](int r, V3 anchorM, V3 anchorF, M3 a, M3 inv) {
        out[r].m = anchorM, out[r].f = anchorF, out[r].a = a, out[r].inv = inv;
    };
    auto segment = [&](V3 m0, V3 m1, V3 f0, V3 f1, float lo, float hi, M3* a, M3* inv) {
        V3 dm = m1 - m0, df = f1 - f0;
        float k = length(dm) > 1e-5f ? std::max(lo, std::min(hi, length(df) / length(dm))) : 1.0f;
        V3 d = normalize(dm);
        M3 r = rotation(dm, df);
        *a = r * stretch(d, k);
        *inv = stretch(d, 1.0f / k) * r.transposed();
    };
    M3 a, inv;
    segment(M[kPelvis], M[kHead], f.joint[kPelvis], f.joint[kHead], 0.75f, 1.33f, &a, &inv);
    for (int r : {kRoot, kPelvis, kSpine, kChest, kNeck, kHead, kEyeL, kEyeR, kHeadTop, kClavL, kClavR})
        make(r, M[kPelvis], f.joint[kPelvis], a, inv);
    struct Link {
        int bone, child, inherit;
    };
    const Link links[] = {
        {kUpperArmL, kForearmL, -1}, {kForearmL, kHandL, -1}, {kHandL, -1, kForearmL},
        {kUpperArmR, kForearmR, -1}, {kForearmR, kHandR, -1}, {kHandR, -1, kForearmR},
        {kThighL, kCalfL, -1}, {kCalfL, kFootL, -1}, {kFootL, kToeL, -1}, {kToeL, -1, kFootL},
        {kThighR, kCalfR, -1}, {kCalfR, kFootR, -1}, {kFootR, kToeR, -1}, {kToeR, -1, kFootR},
    };
    std::map<int, M3> rot;
    for (const Link& l : links) {
        if (l.child >= 0) {
            segment(M[l.bone], M[l.child], f.joint[l.bone], f.joint[l.child], 0.5f, 2.0f, &a, &inv);
            rot[l.bone] = rotation(M[l.child] - M[l.bone], f.joint[l.child] - f.joint[l.bone]);
        } else {
            a = rot[l.inherit], inv = a.transposed();
            rot[l.bone] = a;
        }
        make(l.bone, M[l.bone], f.joint[l.bone], a, inv);
    }
}

V3 closest(V3 p, V3 a, V3 b, V3 c, float* u, float* v, float* w) {
    V3 ab = b - a, ac = c - a, ap = p - a;
    float d1 = dot(ab, ap), d2 = dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return *u = 1, *v = 0, *w = 0, a;
    V3 bp = p - b;
    float d3 = dot(ab, bp), d4 = dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return *u = 0, *v = 1, *w = 0, b;
    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        float t = d1 / (d1 - d3);
        return *u = 1 - t, *v = t, *w = 0, a + ab * t;
    }
    V3 cp = p - c;
    float d5 = dot(ab, cp), d6 = dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return *u = 0, *v = 0, *w = 1, c;
    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        float t = d2 / (d2 - d6);
        return *u = 1 - t, *v = 0, *w = t, a + ac * t;
    }
    float va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        float t = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return *u = 0, *v = 1 - t, *w = t, b + (c - b) * t;
    }
    float den = 1.0f / (va + vb + vc);
    *v = vb * den, *w = vc * den, *u = 1 - *v - *w;
    return a + ab * *v + ac * *w;
}

class Transfer {
public:
    Transfer(const Faith& f, const Bone* bones, const std::vector<V3>& model) : m_f(f) {
        m_pos.resize(f.pos.size());
        for (size_t i = 0; i < f.pos.size(); i++) {
            V3 p;
            for (int k = 0; k < 4; k++) {
                const auto& w = f.weights[i * 4 + size_t(k)];
                if (w.second <= 0) continue;
                const Bone& b = bones[w.first];
                p = p + (b.m + b.inv * (f.pos[i] - b.f)) * w.second;
            }
            m_pos[i] = p;
        }
        V3 lo(1e9f, 1e9f, 1e9f), hi(-1e9f, -1e9f, -1e9f);
        auto grow = [&](V3 v) {
            lo = V3(std::min(lo.x, v.x), std::min(lo.y, v.y), std::min(lo.z, v.z));
            hi = V3(std::max(hi.x, v.x), std::max(hi.y, v.y), std::max(hi.z, v.z));
        };
        for (const V3& v : m_pos) grow(v);
        for (const V3& v : model) grow(v);
        m_lo = lo - V3(0.01f, 0.01f, 0.01f);
        for (int a = 0; a < 3; a++) m_n[a] = std::max(1, std::min(200, int(((hi - lo)[a] + 0.02f) / kCell) + 1));
        m_cells.resize(size_t(m_n[0]) * size_t(m_n[1]) * size_t(m_n[2]));
        for (size_t t = 0; t + 2 < f.tris.size(); t += 3) {
            V3 a = m_pos[f.tris[t]], b = m_pos[f.tris[t + 1]], c = m_pos[f.tris[t + 2]];
            int c0[3], c1[3];
            for (int k = 0; k < 3; k++) {
                c0[k] = cell(std::min({a[k], b[k], c[k]}), k);
                c1[k] = cell(std::max({a[k], b[k], c[k]}), k);
            }
            for (int x = c0[0]; x <= c1[0]; x++)
                for (int y = c0[1]; y <= c1[1]; y++)
                    for (int z = c0[2]; z <= c1[2]; z++) m_cells[index(x, y, z)].push_back(uint32_t(t));
        }
    }

    std::vector<std::pair<int, float>> weights(V3 p) const {
        int c[3];
        for (int k = 0; k < 3; k++) c[k] = cell(p[k], k);
        float best = 1e30f, bu = 1, bv = 0, bw = 0;
        long bestTri = -1;
        int maxR = std::max({m_n[0], m_n[1], m_n[2]});
        for (int r = 0; r <= maxR; r++) {
            if (bestTri >= 0 && best <= float(r - 1) * kCell) break;
            for (int x = c[0] - r; x <= c[0] + r; x++)
                for (int y = c[1] - r; y <= c[1] + r; y++)
                    for (int z = c[2] - r; z <= c[2] + r; z++) {
                        if (std::max({abs(x - c[0]), abs(y - c[1]), abs(z - c[2])}) != r) continue;
                        if (x < 0 || y < 0 || z < 0 || x >= m_n[0] || y >= m_n[1] || z >= m_n[2]) continue;
                        for (uint32_t t : m_cells[index(x, y, z)]) {
                            float u, v, w;
                            V3 q = closest(p, m_pos[m_f.tris[t]], m_pos[m_f.tris[t + 1]], m_pos[m_f.tris[t + 2]], &u, &v, &w);
                            float d = length(q - p);
                            if (d < best) best = d, bu = u, bv = v, bw = w, bestTri = long(t);
                        }
                    }
        }
        std::map<int, float> acc;
        if (bestTri >= 0) {
            const float bary[3] = {bu, bv, bw};
            for (int k = 0; k < 3; k++)
                for (int j = 0; j < 4; j++) {
                    const auto& w = m_f.weights[size_t(m_f.tris[size_t(bestTri) + size_t(k)]) * 4 + size_t(j)];
                    if (w.second > 0) acc[w.first] += w.second * bary[k];
                }
        }
        return top4(acc);
    }

    static std::vector<std::pair<int, float>> top4(const std::map<int, float>& acc) {
        std::vector<std::pair<float, int>> w;
        for (const auto& a : acc)
            if (a.second > 0.001f) w.push_back({a.second, a.first});
        std::sort(w.rbegin(), w.rend());
        if (w.size() > 4) w.resize(4);
        float sum = 0;
        for (const auto& x : w) sum += x.first;
        std::vector<std::pair<int, float>> out;
        for (const auto& x : w) out.push_back({x.second, x.first / sum});
        if (out.empty()) out.push_back({kPelvis, 1.0f});
        return out;
    }

private:
    static constexpr float kCell = 0.06f;
    int cell(float v, int axis) const { return std::max(0, std::min(m_n[axis] - 1, int((v - m_lo[axis]) / kCell))); }
    size_t index(int x, int y, int z) const { return (size_t(z) * size_t(m_n[1]) + size_t(y)) * size_t(m_n[0]) + size_t(x); }

    const Faith& m_f;
    std::vector<V3> m_pos;
    V3 m_lo;
    int m_n[3];
    std::vector<std::vector<uint32_t>> m_cells;
};

std::string partImage(const gltf::Scene& scene, int material, int limit, bool* downscaled) {
    gltf::Material mat;
    if (material >= 0 && size_t(material) < scene.materials.size()) mat = scene.materials[size_t(material)];
    int w = 4, h = 4;
    std::vector<uint8_t> px;
    if (mat.image >= 0 && size_t(mat.image) < scene.images.size()) {
        const std::string& bytes = scene.images[size_t(mat.image)].bytes;
        port::gfx::Image png;
        if (port::gfx::decodePNG(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), png, false))
            w = png.width, h = png.height, px = std::move(png.rgba);
        else if (!port::online::decodeImage(bytes, &w, &h, &px))
            px.clear(), w = h = 4;
    }
    if (px.empty()) px.assign(size_t(w) * size_t(h) * 4, 255);
    bool white = true;
    for (int c = 0; c < 4; c++) white &= fabsf(mat.color[c] - 1.0f) < 0.002f;
    if (!white)
        for (size_t i = 0; i < px.size(); i += 4)
            for (int c = 0; c < 4; c++) {
                float v = float(px[i + size_t(c)]) * std::max(0.0f, std::min(1.0f, mat.color[c]));
                px[i + size_t(c)] = uint8_t(lrintf(v));
            }
    while (std::max(w, h) > limit) {
        int nw = std::max(1, w / 2), nh = std::max(1, h / 2);
        std::vector<uint8_t> out(size_t(nw) * size_t(nh) * 4);
        for (int y = 0; y < nh; y++)
            for (int x = 0; x < nw; x++) {
                float rgb[3] = {0, 0, 0}, a = 0, plainSum[3] = {0, 0, 0};
                for (int dy = 0; dy < 2; dy++)
                    for (int dx = 0; dx < 2; dx++) {
                        int sx = std::min(w - 1, x * 2 + dx), sy = std::min(h - 1, y * 2 + dy);
                        const uint8_t* s = &px[(size_t(sy) * size_t(w) + size_t(sx)) * 4];
                        for (int c = 0; c < 3; c++) rgb[c] += s[c] * s[3], plainSum[c] += s[c];
                        a += s[3];
                    }
                uint8_t* d = &out[(size_t(y) * size_t(nw) + size_t(x)) * 4];
                for (int c = 0; c < 3; c++) d[c] = uint8_t(lrintf(a > 0 ? rgb[c] / a : plainSum[c] / 4));
                d[3] = uint8_t(lrintf(a / 4));
            }
        px.swap(out);
        w = nw, h = nh;
        *downscaled = true;
    }
    std::string png;
    port::gfx::encodePNG(w, h, px.data(), &png);
    return png;
}

}

bool fit(const gltf::Scene& scene, const gltf::Model& faithModel, gltf::Model* out, Report* report, std::string* error) {
    Faith f;
    if (!loadFaith(faithModel, &f, error)) return false;
    Source src;
    gather(scene, &src);
    size_t n = src.pos.size();
    if (!n || src.tris.empty()) return *error = T("в файле нет сетки из треугольников", "the file has no triangle mesh"), false;
    if (n > 65535)
        return *error = T("вершин больше 65535 (", "more than 65535 vertices (") + std::to_string(n) +
                        T("): упростите модель (Decimate в Blender)", "): simplify the model (Decimate in Blender)"), false;
    report->rig = rigName(scene);

    Mapping map;
    mapBones(scene, &map);
    for (int r : kRequired)
        if (map.primary[r] < 0) report->missing.push_back(kRoleNames[r]);
    bool bySkeleton = report->missing.empty();
    auto jointPos = [&](int r) {
        const float* w = scene.nodes[size_t(map.primary[r])].world;
        return V3(w[12], w[13], w[14]);
    };

    V3 L(1, 0, 0), U(0, 1, 0), F(0, 0, 1);
    if (bySkeleton) {
        auto low = [&](int foot, int calf) { return map.primary[foot] >= 0 ? jointPos(foot) : jointPos(calf); };
        V3 up = jointPos(kHead) - (low(kFootL, kCalfL) + low(kFootR, kCalfR)) * 0.5f;
        V3 left = jointPos(kUpperArmL) - jointPos(kUpperArmR);
        V3 legs = jointPos(kThighL) - jointPos(kThighR);
        U = normalize(up);
        L = normalize(left - U * dot(left, U));
        if (length(up) < 1e-6f || length(left - U * dot(left, U)) < 1e-6f * length(left) || dot(legs, L) <= 0) {
            bySkeleton = false;
            report->missing.push_back("?");
            L = V3(1, 0, 0), U = V3(0, 1, 0);
        }
        F = cross(L, U);
    }
    std::vector<V3> p(n);
    V3 lo, hi;
    auto orient = [&](V3 v) { return V3(dot(v, L), dot(v, U), dot(v, F)); };
    auto reorient = [&] {
        lo = V3(1e9f, 1e9f, 1e9f), hi = V3(-1e9f, -1e9f, -1e9f);
        for (size_t i = 0; i < n; i++) {
            p[i] = orient(src.pos[i]);
            lo = V3(std::min(lo.x, p[i].x), std::min(lo.y, p[i].y), std::min(lo.z, p[i].z));
            hi = V3(std::max(hi.x, p[i].x), std::max(hi.y, p[i].y), std::max(hi.z, p[i].z));
        }
    };
    reorient();
    if (!bySkeleton) {
        auto span = [&](int axis) {
            std::vector<float> c(n);
            for (size_t i = 0; i < n; i++) c[i] = axis ? p[i].z : p[i].x;
            std::nth_element(c.begin(), c.begin() + long(n / 2), c.end());
            float mid = c[n / 2];
            return std::min((axis ? hi.z : hi.x) - mid, mid - (axis ? lo.z : lo.x));
        };
        if (span(1) > 1.15f * span(0)) {
            L = V3(0, 0, 1), F = cross(L, U);
            reorient();
        }
    }
    V3 ext = hi - lo;
    if (ext.y <= 1e-6f) return *error = T("модель плоская", "the model is flat"), false;
    if (!bySkeleton && ext.y < 0.5f * std::max(ext.x, ext.z))
        return *error = T("модель лежит: поставьте её вертикально (вверх — ось +Y glTF, в Blender — +Z)",
                          "the model is lying down: stand it upright (up is glTF +Y, +Z in Blender)"), false;

    V3 M[kRoles];
    float scale;
    V3 center;
    if (bySkeleton) {
        for (int r = 0; r < kRoles; r++)
            if (map.primary[r] >= 0) M[r] = orient(jointPos(r));
        float head = M[kHead].y - lo.y;
        if (head <= 1e-6f) return *error = T("голова ниже ног", "the head is below the feet"), false;
        scale = f.joint[kHead].y / head;
        center = V3(M[kPelvis].x, lo.y, M[kPelvis].z);
    } else {
        scale = f.top / ext.y;
        center = V3((lo.x + hi.x) * 0.5f, lo.y, 0);
        V3 c;
        float pelvis = lo.y + ext.y * (f.joint[kPelvis].y / f.top);
        if (slab(p, pelvis, 0.05f * ext.y, [&](const V3& v) { return fabsf(v.x - center.x) < 0.1f * ext.y; }, &c)) center.z = c.z;
        else center.z = (lo.z + hi.z) * 0.5f;
    }
    auto place = [&](V3 v) { return V3((v.x - center.x) * scale, (v.y - center.y) * scale, (v.z - center.z) * scale + f.joint[kPelvis].z); };
    for (V3& v : p) v = place(v);
    if (bySkeleton) {
        for (int r = 0; r < kRoles; r++)
            if (map.primary[r] >= 0) M[r] = place(M[r]);
        const int synth[][3] = {
            {kSpine, kPelvis, kHead}, {kChest, kPelvis, kHead}, {kNeck, kPelvis, kHead}, {kHeadTop, kPelvis, kHead},
            {kEyeL, kPelvis, kHead}, {kEyeR, kPelvis, kHead}, {kClavL, kChest, kUpperArmL}, {kClavR, kChest, kUpperArmR},
            {kHandL, kUpperArmL, kForearmL}, {kHandR, kUpperArmR, kForearmR}, {kFootL, kThighL, kCalfL},
            {kFootR, kThighR, kCalfR}, {kToeL, kCalfL, kFootL}, {kToeR, kCalfR, kFootR},
        };
        for (const auto& s : synth)
            if (map.primary[s[0]] < 0) M[s[0]] = likeFaith(f, M, s[0], s[1], s[2]);
        M[kRoot] = V3(M[kPelvis].x, 0, M[kPelvis].z);
    } else {
        std::vector<V3> s = surface(p, src.tris, 0.02f);
        float vote = 0;
        for (float sg : {1.0f, -1.0f}) {
            auto side = [sg](const V3& v) { return v.x * sg > 0.01f; };
            V3 sole, ankle;
            if (slab(s, 0.015f, 0.02f, side, &sole) && slab(s, f.joint[kFootL].y + 0.03f, 0.03f, side, &ankle)) vote += sole.z - ankle.z;
        }
        if (vote < -0.005f) {
            auto turn = [&](V3 v) { return V3(-v.x, v.y, 2 * f.joint[kPelvis].z - v.z); };
            for (V3& v : p) v = turn(v);
            for (V3& v : s) v = turn(v);
            L = L * -1.0f, F = F * -1.0f;
        }
        if (!shapeJoints(s, f, M, error)) return false;
    }
    report->turned = L.x < 0.99f || U.y < 0.99f;
    report->bySkeleton = bySkeleton;

    if (getenv("MEDECOMP_AUTORIG_DEBUG")) {
        for (int r = 0; r < kRoles; r++)
            fprintf(stderr, "autorig: %-10s %-28s (%6.3f %6.3f %6.3f)  Фейт (%6.3f %6.3f %6.3f)\n", kRoleNames[r],
                    bySkeleton && map.primary[r] >= 0 ? scene.nodes[size_t(map.primary[r])].name.c_str() : "-", M[r].x, M[r].y,
                    M[r].z, f.joint[r].x, f.joint[r].y, f.joint[r].z);
        fprintf(stderr, "autorig: масштаб %.4f, L (%.2f %.2f %.2f) U (%.2f %.2f %.2f), пах Фейт %.3f\n", scale, L.x, L.y, L.z, U.x, U.y, U.z, f.crotch);
    }
    Bone bones[kRoles];
    boneTransforms(f, M, bones);

    std::vector<int> resolved(scene.nodes.size(), -1);
    if (bySkeleton)
        for (size_t i = 0; i < scene.nodes.size(); i++)
            for (int k = int(i), guard = 0; k >= 0 && guard < 1000; k = scene.nodes[size_t(k)].parent, guard++)
                if (map.role[size_t(k)] >= 0) {
                    int r = map.role[size_t(k)];
                    resolved[i] = r == kEyeL || r == kEyeR || r == kHeadTop ? kHead : r == kRoot ? kPelvis : r;
                    break;
                }
    std::set<int> mappedRoles, mergedNodes;
    std::unique_ptr<Transfer> transfer;
    std::vector<std::vector<std::pair<int, float>>> weights(n);
    for (size_t i = 0; i < n; i++) {
        std::map<int, float> acc;
        float lost = 0;
        if (bySkeleton)
            for (const auto& infl : src.influence[i]) {
                int r = resolved[size_t(infl.first)];
                if (r < 0) {
                    lost += infl.second;
                    continue;
                }
                acc[r] += infl.second;
                mappedRoles.insert(r);
                if (map.primary[r] != infl.first && scene.nodes[size_t(infl.first)].mesh < 0) mergedNodes.insert(infl.first);
            }
        if (!bySkeleton || lost > 0.5f) {
            if (!transfer) transfer.reset(new Transfer(f, bones, p));
            weights[i] = transfer->weights(p[i]);
            if (bySkeleton) report->transferred++;
        } else {
            weights[i] = Transfer::top4(acc);
        }
    }
    report->mapped = int(mappedRoles.size());
    if (const char* dump = getenv("MEDECOMP_AUTORIG_DEBUG"); dump && strcmp(dump, "1") != 0)
        if (FILE* out = fopen(dump, "w")) {
            for (size_t i = 0; i < n; i++) fprintf(out, "%.4f %.4f %.4f %d\n", p[i].x, p[i].y, p[i].z, weights[i][0].first);
            fclose(out);
        }
    report->merged = int(mergedNodes.size());

    out->positions.clear();
    out->joints.clear();
    out->weights.clear();
    for (size_t i = 0; i < n; i++) {
        V3 q;
        for (const auto& w : weights[i]) {
            const Bone& b = bones[w.first];
            q = q + (b.f + b.a * (p[i] - b.m)) * w.second;
        }
        out->positions.push_back(q.x), out->positions.push_back(q.y), out->positions.push_back(q.z);
        for (size_t k = 0; k < 4; k++) {
            bool has = k < weights[i].size();
            out->joints.push_back(uint16_t(has ? f.bone[weights[i][k].first] : 0));
            out->weights.push_back(has ? weights[i][k].second : 0.0f);
        }
    }
    out->uvs = src.uv;
    out->bones = faithModel.bones;
    out->inverseBind = faithModel.inverseBind;

    out->indices.clear();
    out->parts.clear();
    for (size_t part = 0; part < src.partMaterial.size(); part++) {
        gltf::Part pt;
        pt.firstIndex = uint32_t(out->indices.size());
        for (size_t t = 0; t < src.triPart.size(); t++)
            if (src.triPart[t] == int(part))
                for (int k = 0; k < 3; k++) out->indices.push_back(src.tris[t * 3 + size_t(k)]);
        pt.indexCount = uint32_t(out->indices.size()) - pt.firstIndex;
        int material = src.partMaterial[part];
        if (material >= 0 && size_t(material) < scene.materials.size())
            pt.alpha = scene.materials[size_t(material)].alpha, pt.doubleSided = scene.materials[size_t(material)].doubleSided;
        if (pt.indexCount) out->parts.push_back(pt);
    }
    for (int limit : {1024, 512, 256}) {
        size_t total = 0;
        report->downscaled = false;
        std::map<int, std::string> done;
        size_t k = 0;
        for (size_t part = 0; part < src.partMaterial.size(); part++) {
            bool used = false;
            for (int t : src.triPart) used |= t == int(part);
            if (!used) continue;
            int material = src.partMaterial[part];
            auto it = done.find(material);
            if (it == done.end()) it = done.insert({material, partImage(scene, material, limit, &report->downscaled)}).first;
            out->parts[k].image = it->second;
            out->parts[k].imageMime = "image/png";
            total += it->second.size();
            k++;
        }
        if (total + n * 48 < (7u << 20)) break;
        report->downscaled = true;
    }
    report->parts = int(out->parts.size());
    return true;
}

}
}
}
