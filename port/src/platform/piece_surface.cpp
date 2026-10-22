#include "piece_surface.h"

#include <math.h>

#include <algorithm>
#include <map>
#include <utility>

#include "m3g/IndexBuffer.h"
#include "m3g/Mesh.h"
#include "m3g/Node.h"
#include "m3g/Transform.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "m3g/m3g.h"

namespace port {
namespace platform {

namespace {

const float kFacing = 0.7f;
const float kSlack = 0.1f;
const float kBin = 0.05f;
const int kTriangles = 8;

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

struct Raw {
    PieceGroup* group;
    std::array<PieceVertex, 3> tri;
};

void readMesh(m3g::Mesh* mesh, const m3g::Transform& toParent, std::map<std::pair<m3g::Appearance*, int>, PieceGroup>* groups,
              std::vector<Raw>* out) {
    m3g::VertexBuffer* vb = mesh->getVertexBuffer();
    if (!vb) return;
    float sb[4];
    m3g::VertexArray* positions = vb->getPositions(sb);
    if (!positions || positions->getComponentCount() != 3) return;
    const int count = positions->getVertexCount();
    std::vector<float> pos, normals, colors, uv[2];
    readArray(positions, &pos);
    m3g::VertexArray* n = vb->getNormals();
    if (n && n->getComponentCount() == 3 && n->getVertexCount() >= count) readArray(n, &normals);
    m3g::VertexArray* c = vb->getColors();
    const int colorComponents = c ? c->getComponentCount() : 0;
    if (c && (colorComponents == 3 || colorComponents == 4) && c->getVertexCount() >= count) {
        std::vector<signed char> raw(size_t(c->getVertexCount() * colorComponents));
        c->get(0, c->getVertexCount(), raw.data());
        colors.resize(raw.size());
        for (size_t i = 0; i < raw.size(); i++) colors[i] = (unsigned char)raw[i] / 255.0f;
    }
    float uvScaleBias[2][4];
    int units = 0, uvComponents[2] = {};
    for (int u = 0; u < 2; u++) {
        m3g::VertexArray* t = vb->getTexCoords(u, uvScaleBias[u]);
        if (!t || t->getComponentCount() < 2 || t->getVertexCount() < count) break;
        readArray(t, &uv[u]);
        uvComponents[u] = t->getComponentCount();
        units = u + 1;
    }
    const int defaultColor = vb->getDefaultColor();
    std::vector<PieceVertex> verts(static_cast<size_t>(count));
    for (int i = 0; i < count; i++) {
        PieceVertex& v = verts[size_t(i)];
        float p[4] = {pos[size_t(i * 3)] * sb[0] + sb[1], pos[size_t(i * 3 + 1)] * sb[0] + sb[2], pos[size_t(i * 3 + 2)] * sb[0] + sb[3], 1};
        toParent.transform(p, 4);
        std::copy(p, p + 3, v.pos);
        if (!normals.empty()) {
            float q[4] = {normals[size_t(i * 3)], normals[size_t(i * 3 + 1)], normals[size_t(i * 3 + 2)], 0};
            toParent.transform(q, 4);
            float len = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);
            for (int k = 0; k < 3; k++) v.normal[k] = len > 0 ? q[k] / len : 0;
        }
        if (!colors.empty()) {
            for (int k = 0; k < 3; k++) v.color[k] = colors[size_t(i * colorComponents + k)];
            v.color[3] = colorComponents == 4 ? colors[size_t(i * colorComponents + 3)] : 1.0f;
        } else {
            v.color[0] = (defaultColor >> 16 & 0xff) / 255.0f, v.color[1] = (defaultColor >> 8 & 0xff) / 255.0f;
            v.color[2] = (defaultColor & 0xff) / 255.0f, v.color[3] = (defaultColor >> 24 & 0xff) / 255.0f;
        }
        for (int u = 0; u < units; u++) {
            const int k = uvComponents[u];
            v.uv[u][0] = uv[u][size_t(i * k)] * uvScaleBias[u][0] + uvScaleBias[u][1];
            v.uv[u][1] = uv[u][size_t(i * k + 1)] * uvScaleBias[u][0] + uvScaleBias[u][2];
        }
    }
    for (int s = 0; s < mesh->getSubmeshCount(); s++) {
        m3g::IndexBuffer* ib = mesh->getIndexBuffer(s);
        m3g::Appearance* look = mesh->getAppearance(s);
        if (!ib || !look || ib->getPrimitiveType() != kTriangles) continue;
        std::vector<int> idx;
        const midp::array<unsigned short>& explicitIdx = ib->getExplicitIndices();
        if (explicitIdx.m_length > 0) {
            for (int i = 0; i < explicitIdx.m_length; i++) idx.push_back(explicitIdx[i]);
        } else if (ib->isImplicit()) {
            for (int i = 0; i < ib->getPrimitiveCount() * 3; i++) idx.push_back(ib->getFirstIndex() + i);
        }
        PieceGroup& g = (*groups)[{look, (normals.empty() ? 0 : 4) | units}];
        g.look = look, g.normals = !normals.empty(), g.units = units;
        for (size_t i = 0; i + 2 < idx.size(); i += 3) {
            if (idx[i] >= count || idx[i + 1] >= count || idx[i + 2] >= count) continue;
            out->push_back({&g, {verts[size_t(idx[i])], verts[size_t(idx[i + 1])], verts[size_t(idx[i + 2])]}});
        }
    }
}

void walk(m3g::Node* node, m3g::Node* top, const m3g::Transform& own, std::map<std::pair<m3g::Appearance*, int>, PieceGroup>* groups,
          std::vector<Raw>* out) {
    if (m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node)) {
        m3g::Transform t(&own);
        m3g::Transform local;
        if (node == top || mesh->getTransformTo(top, &local)) {
            if (node != top) t.postMultiply(&local);
            readMesh(mesh, t, groups, out);
        }
    }
    if (m3g::Group* group = m3g::m3g_cast<m3g::Group>(node))
        for (int i = 0; i < group->getChildCount(); i++) walk(group->getChild(i), top, own, groups, out);
}

void normalOf(const std::array<PieceVertex, 3>& t, float* n, float* area) {
    const float* a = t[0].pos;
    const float* b = t[1].pos;
    const float* c = t[2].pos;
    float u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, v[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    n[0] = u[1] * v[2] - u[2] * v[1], n[1] = u[2] * v[0] - u[0] * v[2], n[2] = u[0] * v[1] - u[1] * v[0];
    float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    *area = len / 2;
    for (int k = 0; k < 3; k++) n[k] = len > 0 ? n[k] / len : 0;
}

bool plane(const std::vector<Raw>& raw, int axis, float sign, float* at) {
    std::map<long, float> bins;
    for (const Raw& r : raw) {
        float n[3], area;
        normalOf(r.tri, n, &area);
        if (n[axis] * sign < kFacing || area <= 0) continue;
        float c = (r.tri[0].pos[axis] + r.tri[1].pos[axis] + r.tri[2].pos[axis]) / 3;
        bins[lrintf(c / kBin)] += area;
    }
    if (bins.empty()) return false;
    auto best = std::max_element(bins.begin(), bins.end(), [](const std::pair<const long, float>& a, const std::pair<const long, float>& b) {
        return a.second < b.second;
    });
    *at = best->first * kBin;
    return true;
}

void add(std::vector<PieceGroup>* out, std::map<PieceGroup*, size_t>* index, const Raw& r) {
    auto it = index->find(r.group);
    if (it == index->end()) {
        PieceGroup g;
        g.look = r.group->look, g.normals = r.group->normals, g.units = r.group->units;
        out->push_back(g);
        it = index->insert({r.group, out->size() - 1}).first;
    }
    (*out)[it->second].tris.push_back(r.tri);
}

}

bool readPieceSurface(m3g::Node* node, PieceSurface* out) {
    *out = PieceSurface();
    if (!node) return false;
    m3g::Transform own;
    node->getCompositeTransform(&own);
    std::map<std::pair<m3g::Appearance*, int>, PieceGroup> groups;
    std::vector<Raw> raw;
    walk(node, node, own, &groups, &raw);
    if (raw.empty()) return false;
    PiecePlanes& p = out->planes;
    for (int k = 0; k < 3; k++) p.lo[k] = 1e30f, p.hi[k] = -1e30f;
    for (const Raw& r : raw)
        for (const PieceVertex& v : r.tri)
            for (int k = 0; k < 3; k++) p.lo[k] = std::min(p.lo[k], v.pos[k]), p.hi[k] = std::max(p.hi[k], v.pos[k]);
    p.hasFront = plane(raw, 2, 1, &p.front);
    p.hasRoof = plane(raw, 1, 1, &p.roof);
    p.hasLeft = plane(raw, 0, -1, &p.left);
    p.hasRight = plane(raw, 0, 1, &p.right);
    if (!p.hasFront) p.front = p.hi[2];
    if (!p.hasRoof) p.roof = p.hi[1];
    std::map<PieceGroup*, size_t> front, roof, left, right;
    for (const Raw& r : raw) {
        float n[3], area;
        normalOf(r.tri, n, &area);
        if (area <= 0) continue;
        float c[3];
        for (int k = 0; k < 3; k++) c[k] = (r.tri[0].pos[k] + r.tri[1].pos[k] + r.tri[2].pos[k]) / 3;
        if (p.hasFront && c[2] >= p.front - kFrontBand && n[2] > -kFacing) add(&out->front, &front, r);
        if (p.hasRoof && n[1] >= kFacing && fabsf(c[1] - p.roof) <= kSlack) add(&out->roof, &roof, r);
        if (p.hasLeft && n[0] <= -kFacing && fabsf(c[0] - p.left) <= kSlack) add(&out->left, &left, r);
        if (p.hasRight && n[0] >= kFacing && fabsf(c[0] - p.right) <= kSlack) add(&out->right, &right, r);
    }
    for (int k = 0; k < 2; k++) p.faceLo[k] = 1e30f, p.faceHi[k] = -1e30f;
    float wallLo = 1e30f, wallHi = -1e30f;
    for (const PieceGroup& g : out->front)
        for (const auto& tri : g.tris) {
            float n[3], area, cy = (tri[0].pos[1] + tri[1].pos[1] + tri[2].pos[1]) / 3, cz = (tri[0].pos[2] + tri[1].pos[2] + tri[2].pos[2]) / 3;
            normalOf(tri, n, &area);
            const bool wall = n[2] >= kFacing && fabsf(cz - p.front) <= kSlack && cy <= p.roof + kSlack;
            for (const PieceVertex& v : tri) {
                for (int k = 0; k < 2; k++) p.faceLo[k] = std::min(p.faceLo[k], v.pos[k]), p.faceHi[k] = std::max(p.faceHi[k], v.pos[k]);
                if (wall) wallLo = std::min(wallLo, v.pos[0]), wallHi = std::max(wallHi, v.pos[0]);
            }
        }
    if (wallLo < wallHi) p.faceLo[0] = wallLo, p.faceHi[0] = wallHi;
    if (out->front.empty())
        for (int k = 0; k < 2; k++) p.faceLo[k] = p.lo[k], p.faceHi[k] = p.hi[k];
    return true;
}

}
}
