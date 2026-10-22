#include "heroine_model.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <map>

#include "game/M3GAssets.h"
#include "../gfx/png.h"
#include "m3g/Appearance.h"
#include "m3g/CompositingMode.h"
#include "m3g/Image2D.h"
#include "m3g/IndexBuffer.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Mesh.h"
#include "m3g/Node.h"
#include "m3g/PolygonMode.h"
#include "m3g/SkinnedMesh.h"
#include "m3g/Texture2D.h"
#include "m3g/Transform.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "m3g/m3g.h"
#include "port/online.h"

namespace port {
namespace platform {
namespace heroine {

namespace {

const char* const kBones[] = {
    "root",        "pelvis",      "thigh_l",     "calf_l",      "foot_l",      "toe_l",       "spine",
    "chest",       "neck",        "head",        "eye_l",       "eye_r",       "head_top",    "clavicle_l",
    "upperarm_l",  "forearm_l",   "hand_l",      "clavicle_r",  "upperarm_r",  "forearm_r",   "hand_r",
    "thigh_r",     "calf_r",      "foot_r",      "toe_r",
};
const int kBoneCount = int(sizeof kBones / sizeof kBones[0]);

m3g::SkinnedMesh* findSkinned(m3g::Node* node) {
    if (!node) return nullptr;
    if (m3g::SkinnedMesh* s = m3g::m3g_cast<m3g::SkinnedMesh>(node)) return s;
    if (m3g::m3g_cast<m3g::Mesh>(node)) return nullptr;
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (!group) return nullptr;
    for (int i = 0; i < group->getChildCount(); i++)
        if (m3g::SkinnedMesh* s = findSkinned(group->getChild(i))) return s;
    return nullptr;
}

m3g::Group* parentOf(m3g::Node* node, m3g::Node* child) {
    m3g::Group* group = node && !m3g::m3g_cast<m3g::Mesh>(node) ? m3g::m3g_cast<m3g::Group>(node) : nullptr;
    if (!group) return nullptr;
    for (int i = 0; i < group->getChildCount(); i++) {
        if (group->getChild(i) == child) return group;
        if (m3g::Group* g = parentOf(group->getChild(i), child)) return g;
    }
    return nullptr;
}

void bonesOf(m3g::Node* node, int parent, std::vector<m3g::Node*>* out, std::vector<int>* parents) {
    out->push_back(node);
    parents->push_back(parent);
    int self = int(out->size()) - 1;
    if (m3g::Group* group = m3g::m3g_cast<m3g::Group>(node))
        for (int i = 0; i < group->getChildCount(); i++) bonesOf(group->getChild(i), self, out, parents);
}

void toColumns(const m3g::Transform& t, float* out) {
    float m[16];
    t.get(m);
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) out[c * 4 + r] = m[r * 4 + c];
}

void readArray(m3g::VertexArray* array, std::vector<float>* out) {
    int n = array->getVertexCount() * array->getComponentCount();
    out->resize(size_t(n));
    switch (array->getComponentType()) {
    case 1: {
        std::vector<signed char> v(static_cast<size_t>(n));
        array->get(0, array->getVertexCount(), v.data());
        for (int i = 0; i < n; i++) (*out)[size_t(i)] = v[size_t(i)];
        break;
    }
    case 2: {
        std::vector<short> v(static_cast<size_t>(n));
        array->get(0, array->getVertexCount(), v.data());
        for (int i = 0; i < n; i++) (*out)[size_t(i)] = v[size_t(i)];
        break;
    }
    default:
        array->get(0, array->getVertexCount(), out->data());
    }
}

bool close(const float* a, const float* b, float eps) {
    for (int i = 0; i < 16; i++)
        if (fabsf(a[i] - b[i]) > eps) return false;
    return true;
}

}

void hideOthers(m3g::Node* node, m3g::Node* figure) {
    if (!node || node == figure) return;
    int id = node->getUserID();
    if ((id >= 103 && id <= 110)) return;
    m3g::Mesh* mesh = m3g::m3g_cast<m3g::SkinnedMesh>(node);
    if (!mesh) mesh = m3g::m3g_cast<m3g::Mesh>(node);
    if (mesh) {
        bool mutableWas = mesh->m_mutable;
        mesh->m_mutable = true;
        for (int i = 0; i < mesh->getSubmeshCount(); i++) {
            midp::array<int> empty(3);
            empty[0] = empty[1] = empty[2] = 0;
            mesh->setIndexBuffer(i, new m3g::IndexBuffer(8, 1, empty));
        }
        mesh->m_mutable = mutableWas;
        fprintf(stderr, "medecomp: своя модель — сетка Фейт (%d вершин) не рисуется\n",
                mesh->getVertexBuffer() ? mesh->getVertexBuffer()->getVertexCount() : 0);
        if (m3g::SkinnedMesh* skinned = m3g::m3g_cast<m3g::SkinnedMesh>(node)) hideOthers(skinned->getSkeleton(), figure);
        return;
    }
    if (m3g::Group* group = m3g::m3g_cast<m3g::Group>(node))
        for (int i = 0; i < group->getChildCount(); i++) hideOthers(group->getChild(i), figure);
}

const char* boneName(int index) { return index >= 0 && index < kBoneCount ? kBones[index] : ""; }
int boneCount() { return kBoneCount; }

bool faithTemplate(m3g::Node* figure, const std::string& png, gltf::Model* out, std::string* error) {
    m3g::SkinnedMesh* mesh = findSkinned(figure);
    if (!mesh || !mesh->getSkeleton()) return *error = gltf::T("нет сетки со скелетом", "no skinned mesh"), false;
    std::vector<m3g::Node*> bones;
    std::vector<int> parents;
    bonesOf(mesh->getSkeleton(), -1, &bones, &parents);
    if (int(bones.size()) != kBoneCount)
        return *error = gltf::T("у скелета костей: ", "skeleton bones: ") + std::to_string(bones.size()) + gltf::T(", ждали ", ", expected ") + std::to_string(kBoneCount), false;
    for (int b = 0; b < kBoneCount; b++) {
        gltf::Bone bone;
        bone.name = kBones[b];
        bone.parent = parents[size_t(b)];
        m3g::Transform local;
        m3g::Node* parent = bone.parent < 0 ? static_cast<m3g::Node*>(mesh) : bones[size_t(bone.parent)];
        bones[size_t(b)]->getTransformTo(parent, &local);
        toColumns(local, bone.matrix);
        out->bones.push_back(bone);
        m3g::Transform bind;
        if (m3g::Transform* captured = mesh->getBoneTransform(bones[size_t(b)])) bind.set(captured);
        else mesh->getTransformTo(bones[size_t(b)], &bind);
        float m[16];
        toColumns(bind, m);
        out->inverseBind.insert(out->inverseBind.end(), m, m + 16);
    }
    m3g::VertexBuffer* vb = mesh->getVertexBuffer();
    float sb[4];
    std::vector<float> raw;
    readArray(vb->getPositions(sb), &raw);
    int n = vb->getVertexCount();
    for (int i = 0; i < n; i++)
        for (int c = 0; c < 3; c++) out->positions.push_back(raw[size_t(i) * 3 + size_t(c)] * sb[0] + sb[1 + c]);
    if (m3g::VertexArray* uv = vb->getTexCoords(0, sb)) {
        readArray(uv, &raw);
        int comps = uv->getComponentCount();
        for (int i = 0; i < n; i++)
            for (int c = 0; c < 2; c++) out->uvs.push_back(raw[size_t(i) * size_t(comps) + size_t(c)] * sb[0] + sb[1 + c]);
    } else {
        out->uvs.assign(size_t(n) * 2, 0.0f);
    }
    for (int i = 0; i < n; i++) {
        std::vector<std::pair<int, int>> w;
        for (int b = 0; b < kBoneCount; b++)
            if (midp::array<int>* weights = mesh->m_weights ? mesh->getWeights(bones[size_t(b)]) : nullptr)
                if (int v = abs((*weights)[i])) w.push_back({v, b});
        std::sort(w.rbegin(), w.rend());
        w.resize(4, {0, 0});
        float sum = float(w[0].first + w[1].first + w[2].first + w[3].first);
        for (int k = 0; k < 4; k++) {
            out->joints.push_back(uint16_t(w[size_t(k)].second));
            out->weights.push_back(sum > 0 ? float(w[size_t(k)].first) / sum : (k == 0 ? 1.0f : 0.0f));
        }
    }
    for (int s = 0; s < mesh->getSubmeshCount(); s++) {
        m3g::IndexBuffer* ib = mesh->getIndexBuffer(s);
        if (!ib) continue;
        midp::array<int> idx(ib->getIndexCount());
        ib->getIndices(idx);
        for (int i = 0; i + 2 < idx.m_length; i += 3)
            for (int k = 0; k < 3; k++) out->indices.push_back(uint32_t(idx[i + k]));
    }
    gltf::Part part;
    part.indexCount = uint32_t(out->indices.size());
    part.image = png;
    part.imageMime = "image/png";
    out->parts.push_back(part);
    return true;
}

bool torsoMask(m3g::Node* figure, int w, int h, std::vector<uint8_t>* mask) {
    m3g::SkinnedMesh* mesh = findSkinned(figure);
    if (!mesh || !mesh->getSkeleton() || !mesh->m_weights) return false;
    std::vector<m3g::Node*> bones;
    std::vector<int> parents;
    bonesOf(mesh->getSkeleton(), -1, &bones, &parents);
    if (int(bones.size()) != kBoneCount) return false;
    m3g::VertexBuffer* vb = mesh->getVertexBuffer();
    float sb[4];
    m3g::VertexArray* uvArray = vb->getTexCoords(0, sb);
    if (!uvArray) return false;
    std::vector<float> raw;
    readArray(uvArray, &raw);
    int n = vb->getVertexCount(), comps = uvArray->getComponentCount();
    std::vector<int> top(size_t(n), -1), best(size_t(n), 0);
    for (int b = 0; b < kBoneCount; b++)
        if (midp::array<int>* weights = mesh->getWeights(bones[size_t(b)]))
            for (int i = 0; i < n; i++)
                if (abs((*weights)[i]) > best[size_t(i)]) best[size_t(i)] = abs((*weights)[i]), top[size_t(i)] = b;
    auto torso = [](int b) { return b == 6 || b == 7; };
    mask->assign(size_t(w) * size_t(h), 0);
    for (int s = 0; s < mesh->getSubmeshCount(); s++) {
        m3g::IndexBuffer* ib = mesh->getIndexBuffer(s);
        if (!ib) continue;
        midp::array<int> idx(ib->getIndexCount());
        ib->getIndices(idx);
        for (int t = 0; t + 2 < idx.m_length; t += 3) {
            int v[3] = {idx[t], idx[t + 1], idx[t + 2]};
            if (!torso(top[size_t(v[0])]) || !torso(top[size_t(v[1])]) || !torso(top[size_t(v[2])])) continue;
            float x[3], y[3];
            for (int k = 0; k < 3; k++) {
                x[k] = (raw[size_t(v[k]) * size_t(comps)] * sb[0] + sb[1]) * float(w);
                y[k] = (raw[size_t(v[k]) * size_t(comps) + 1] * sb[0] + sb[2]) * float(h);
            }
            int x0 = std::max(0, int(floorf(std::min({x[0], x[1], x[2]}))) - 1);
            int x1 = std::min(w - 1, int(ceilf(std::max({x[0], x[1], x[2]}))) + 1);
            int y0 = std::max(0, int(floorf(std::min({y[0], y[1], y[2]}))) - 1);
            int y1 = std::min(h - 1, int(ceilf(std::max({y[0], y[1], y[2]}))) + 1);
            float area = (x[1] - x[0]) * (y[2] - y[0]) - (x[2] - x[0]) * (y[1] - y[0]);
            if (fabsf(area) < 1e-6f) continue;
            for (int py = y0; py <= y1; py++)
                for (int px = x0; px <= x1; px++) {
                    float cx = px + 0.5f, cy = py + 0.5f;
                    float a = ((x[1] - cx) * (y[2] - cy) - (x[2] - cx) * (y[1] - cy)) / area;
                    float b = ((x[2] - cx) * (y[0] - cy) - (x[0] - cx) * (y[2] - cy)) / area;
                    float c = 1.0f - a - b;
                    const float e = -0.02f;
                    if (a >= e && b >= e && c >= e) (*mask)[size_t(py) * size_t(w) + size_t(px)] = 1;
                }
        }
    }
    return true;
}

bool checkModel(const gltf::Model& model, std::vector<int>* boneMap, std::string* error) {
    size_t n = model.vertexCount();
    if (model.uvs.size() != n * 2 || model.joints.size() != n * 4 || model.weights.size() != n * 4)
        return *error = gltf::T("у модели разное число вершин, развёрток и весов", "the model has different counts of vertices, UVs and weights"), false;
    for (uint32_t i : model.indices)
        if (i >= n) return *error = gltf::T("индекс за концом вершин", "an index is past the last vertex"), false;
    std::map<std::string, int> faith;
    for (int b = 0; b < kBoneCount; b++) faith[kBones[b]] = b;
    boneMap->assign(model.bones.size(), -1);
    for (size_t b = 0; b < model.bones.size(); b++) {
        auto it = faith.find(model.bones[b].name);
        if (it == faith.end())
            return *error = gltf::T("кость «", "bone \"") + model.bones[b].name + gltf::T("» не из шаблона (faith-template.glb): скелет — как у Фейт", "\" is not in the template (faith-template.glb): the skeleton must be Faith's"), false;
        (*boneMap)[b] = it->second;
    }
    if (model.vertexCount() > 65535) return *error = gltf::T("вершин больше 65535", "more than 65535 vertices"), false;
    return true;
}

bool wearModel(m3g::Node* figure, const gltf::Model& model, m3g::Image2D* image, std::string* error) {
    std::vector<int> map;
    if (!checkModel(model, &map, error)) return false;
    m3g::SkinnedMesh* old = findSkinned(figure);
    if (!old || !old->getSkeleton()) return *error = gltf::T("у фигуры нет сетки со скелетом", "the figure has no skinned mesh"), false;
    std::vector<m3g::Node*> bones;
    std::vector<int> parents;
    bonesOf(old->getSkeleton(), -1, &bones, &parents);
    if (int(bones.size()) != kBoneCount) return *error = gltf::T("скелет фигуры не как у Фейт", "the figure's skeleton is not Faith's"), false;
    for (size_t b = 0; b < model.bones.size(); b++) {
        m3g::Transform bind;
        if (m3g::Transform* captured = old->getBoneTransform(bones[size_t(map[b])])) bind.set(captured);
        else old->getTransformTo(bones[size_t(map[b])], &bind);
        float m[16];
        toColumns(bind, m);
        if (!close(m, &model.inverseBind[b * 16], 0.02f))
            return *error = gltf::T("кость «", "bone \"") + model.bones[b].name + gltf::T("» сдвинута или повёрнута: верните позу шаблона", "\" is moved or rotated: restore the template pose"), false;
    }
    int n = int(model.vertexCount());
    auto quantize = [](const std::vector<float>& v, int comps, int count, float* scale, float* bias,
                       std::vector<short>* out) {
        float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
        for (int i = 0; i < count; i++)
            for (int c = 0; c < comps; c++)
                lo[c] = std::min(lo[c], v[size_t(i * comps + c)]), hi[c] = std::max(hi[c], v[size_t(i * comps + c)]);
        float extent = 0;
        for (int c = 0; c < comps; c++) extent = std::max(extent, hi[c] - lo[c]), bias[c] = (lo[c] + hi[c]) * 0.5f;
        *scale = extent > 0 ? extent / 65000.0f : 1.0f;
        out->resize(size_t(count * comps));
        for (int i = 0; i < count; i++)
            for (int c = 0; c < comps; c++)
                (*out)[size_t(i * comps + c)] = short(lrintf((v[size_t(i * comps + c)] - bias[c]) / *scale));
    };
    float scale, bias[3] = {0, 0, 0};
    std::vector<short> q;
    m3g::VertexBuffer* vb = new m3g::VertexBuffer();
    quantize(model.positions, 3, n, &scale, bias, &q);
    m3g::VertexArray* positions = new m3g::VertexArray(n, 3, 2);
    positions->set(0, n, q.data());
    vb->setPositions(positions, scale, bias);
    float uvBias[3] = {0, 0, 0};
    quantize(model.uvs, 2, n, &scale, uvBias, &q);
    m3g::VertexArray* uvs = new m3g::VertexArray(n, 2, 2);
    uvs->set(0, n, q.data());
    vb->setTexCoords(0, uvs, scale, uvBias);
    if (m3g::VertexBuffer* was = old->getVertexBuffer()) vb->setDefaultColor(was->getDefaultColor());

    m3g::SkinnedMesh* mesh = new m3g::SkinnedMesh();
    mesh->m_mutable = true;
    mesh->setVertexBuffer(vb);
    std::vector<gltf::Part> parts = model.parts;
    if (parts.empty()) parts.push_back(gltf::Part()), parts[0].indexCount = uint32_t(model.indices.size());
    mesh->setSubmeshCount(int(parts.size()));
    m3g::Appearance* base = old->getAppearance(0);
    m3g::Texture2D* texture = base ? base->getTexture(0) : nullptr;
    for (size_t p = 0; p < parts.size(); p++) {
        const gltf::Part& part = parts[p];
        midp::array<int> indices(int(part.indexCount));
        for (uint32_t i = 0; i < part.indexCount; i++) indices[int(i)] = int(model.indices[part.firstIndex + i]);
        mesh->setIndexBuffer(int(p), new m3g::IndexBuffer(8, int(part.indexCount / 3), indices));
        if (!base) continue;
        m3g::Image2D* partImage = image;
        if (!partImage && !part.image.empty()) {
            port::gfx::Image png;
            int w = 0, h = 0;
            std::vector<uint8_t> rgba;
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(part.image.data());
            if (port::gfx::decodePNG(bytes, part.image.size(), png, false))
                w = png.width, h = png.height, rgba = std::move(png.rgba);
            else if (!port::online::decodeImage(part.image, &w, &h, &rgba))
                fprintf(stderr, "medecomp: своя модель — картинку части %zu не прочитать\n", p);
            if (!rgba.empty()) {
                partImage = new m3g::Image2D();
                partImage->set(100, w, h);
                partImage->commit(rgba.data());
            }
        }
        m3g::Appearance* look = base;
        if (partImage && texture)
            look = M3GAssets::createTexturedAppearance(base, partImage, texture->getLevelFilter(), texture->getImageFilter(),
                                                       texture->getBlending());
        if (part.alpha || part.doubleSided) {
            if (look == base) look = static_cast<m3g::Appearance*>(base->duplicate());
            if (part.alpha) {
                m3g::CompositingMode* was = look->getCompositingMode();
                int flags = was ? (was->isDepthTestEnabled() ? 1 : 0) | (was->isDepthWriteEnabled() ? 2 : 0) |
                                      (was->isColorWriteEnabled() ? 4 : 0) | (was->isAlphaWriteEnabled() ? 8 : 0)
                                : 1 | 2 | 4 | 8;
                look->setCompositingMode(M3GAssets::createCompositingMode(flags, 0.5f, was ? was->getBlending() : 68));
            }
            if (part.doubleSided) {
                m3g::PolygonMode* was = look->getPolygonMode();
                m3g::PolygonMode* mode = new m3g::PolygonMode();
                if (was) {
                    mode->setWinding(was->getWinding());
                    mode->setShading(was->getShading());
                    mode->setTwoSidedLightingEnable(was->isTwoSidedLightingEnabled());
                    mode->setLocalCameraLightingEnable(was->isLocalCameraLightingEnabled());
                    mode->setPerspectiveCorrectionEnable(was->isPerspectiveCorrectionEnabled());
                }
                mode->setCulling(162);
                look->setPolygonMode(mode);
            }
        }
        mesh->setAppearance(int(p), look);
    }
    mesh->preparePositionsForSkinning();
    mesh->setSkeleton(old->getSkeleton());
    mesh->setLegacy(true);
    for (int i = 0; i < n; i++)
        for (int k = 0; k < 4; k++) {
            float w = model.weights[size_t(i) * 4 + size_t(k)];
            if (w <= 0.0f) continue;
            mesh->addTransform(bones[size_t(map[model.joints[size_t(i) * 4 + size_t(k)]])], int(lrintf(w * 1000.0f)), i, 1);
        }
    for (int b = 0; b < kBoneCount; b++)
        if (mesh->m_bindTransforms && mesh->m_bindTransforms->containsKey(bones[size_t(b)]))
            if (m3g::Transform* captured = old->getBoneTransform(bones[size_t(b)]))
                mesh->m_bindTransforms->get(bones[size_t(b)])->set(captured);
    m3g::Loader_SkinnedMesh_warmCache(mesh);
    m3g::Group* parent = parentOf(figure, old);
    if (!parent) return *error = gltf::T("у сетки нет родителя", "the mesh has no parent"), false;
    parent->removeChild(old);
    parent->addChild(mesh);
    return true;
}

}
}
}
