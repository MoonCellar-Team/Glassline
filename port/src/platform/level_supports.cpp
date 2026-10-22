#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <map>
#include <vector>

#include "levels.h"
#include "piece_surface.h"

#include "game/CollShape.h"
#include "game/M3GAssets.h"
#include "game/MEdgeMap.h"
#include "game/MapPalette.h"
#include "m3g/Appearance.h"
#include "m3g/IndexBuffer.h"
#include "m3g/Mesh.h"
#include "m3g/Node.h"
#include "m3g/PolygonMode.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "runtime/midp/array.h"

extern "C" int medecomp_player_level_supports(void);
m3g::Node* medecomp_piece_template(MapPalette& palette, int userId);

namespace {

using port::platform::PieceGroup;
using port::platform::PieceSurface;
using port::platform::PieceVertex;

const int kShownMaterial = 7;
const int kDarkMaterial = 6;
const int kNumLayers = 4;
const float kMinStep = 0.25f;
const int kMaxTiles = 2000;
const int kMeshVertices = 65535;

std::vector<int> shownShapes() {
    port::platform::SharedLevel level;
    if (!port::platform::currentSharedLevel(&level)) return {};
    const std::vector<unsigned char>& d = level.map;
    size_t pos = 0;
    auto bytes = [&](int n) {
        unsigned v = 0;
        for (int i = 0; i < n && pos < d.size(); i++) v = v << 8 | d[pos++];
        return v;
    };
    bytes(4 + 1 + 4 + 12);
    int sections = int(int16_t(bytes(2))), shapes = 0;
    for (int i = 0; i < sections * kNumLayers; i++) {
        bytes(2);
        shapes = int(int16_t(bytes(2)));
    }
    std::vector<int> out(size_t(std::max(0, shapes)), -1);
    bool any = false;
    for (int i = 0; i < shapes && pos + 21 <= d.size(); i++) {
        bytes(16 + 1);
        const int material = int(bytes(4)), index = port::platform::shapeMaterialIndex(material);
        out[size_t(i)] = material;
        any = any || index == kShownMaterial || index == kDarkMaterial || port::platform::shapeSurface(material) != 0;
    }
    return any ? out : std::vector<int>();
}

struct Colors {
    unsigned top, front, side, slope, back;
};
const Colors kWhite = {0xf4f6f8, 0xdfe3e8, 0xc6ccd3, 0xeceff3, 0xaeb5bd};
const Colors kDark = {0x3c4046, 0x22252a, 0x1a1c20, 0x34373c, 0x121315};

enum Face { kTop = 1, kFront = 2, kLeft = 4, kRight = 8, kBack = 16, kBottom = 32, kSlope = 64, kAll = 127 };

struct Builder {
    std::vector<float> pos;
    std::vector<unsigned char> col;
    std::vector<int> idx;

    void vertex(float x, float y, float z, unsigned rgb) {
        pos.insert(pos.end(), {x, y, z});
        col.insert(col.end(), {(unsigned char)(rgb >> 16), (unsigned char)(rgb >> 8), (unsigned char)rgb, 255});
    }
    void face(std::initializer_list<std::array<float, 3>> pts, unsigned rgb) {
        const int first = int(pos.size() / 3);
        for (const auto& p : pts) vertex(p[0], p[1], p[2], rgb);
        for (int i = 1; i + 1 < int(pts.size()); i++) idx.insert(idx.end(), {first, first + i, first + i + 1});
    }
    void box(float x0, float y0, float z0, float x1, float y1, float z1, const Colors& k, int faces = kAll) {
        if (faces & kTop) face({{x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}}, k.top);
        if (faces & kFront) face({{x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}}, k.front);
        if (faces & kLeft) face({{x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}}, k.side);
        if (faces & kRight) face({{x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}}, k.side);
        if (faces & kBack) face({{x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}}, k.back);
        if (faces & kBottom) face({{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}}, k.back);
    }
    void ramp(float x0, float y0, float z0, float x1, float y1, float z1, bool highAtMin, const Colors& k, int faces = kAll) {
        const float hx = highAtMin ? x0 : x1, lx = highAtMin ? x1 : x0;
        if (faces & kFront) face({{x0, y0, z1}, {x1, y0, z1}, {hx, y1, z1}}, k.front);
        if (faces & kBack) face({{x0, y0, z0}, {hx, y1, z0}, {x1, y0, z0}}, k.back);
        if (faces & (highAtMin ? kLeft : kRight)) face({{hx, y0, z0}, {hx, y0, z1}, {hx, y1, z1}, {hx, y1, z0}}, k.side);
        if (faces & kSlope) face({{hx, y1, z1}, {lx, y0, z1}, {lx, y0, z0}, {hx, y1, z0}}, k.slope);
        if (faces & kBottom) face({{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}}, k.back);
    }
};

struct Plane {
    float a[3], d;
};
struct Affine {
    float m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float n[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float t[3] = {};
};

PieceVertex lerp(const PieceVertex& a, const PieceVertex& b, float k) {
    PieceVertex v;
    const float* pa = &a.pos[0];
    const float* pb = &b.pos[0];
    float* pv = &v.pos[0];
    for (size_t i = 0; i < sizeof(PieceVertex) / sizeof(float); i++) pv[i] = pa[i] + (pb[i] - pa[i]) * k;
    return v;
}

void clip(std::vector<PieceVertex>* poly, const std::vector<Plane>& planes) {
    std::vector<PieceVertex> out;
    for (const Plane& p : planes) {
        if (poly->size() < 3) break;
        out.clear();
        for (size_t i = 0; i < poly->size(); i++) {
            const PieceVertex& a = (*poly)[i];
            const PieceVertex& b = (*poly)[(i + 1) % poly->size()];
            const float da = p.a[0] * a.pos[0] + p.a[1] * a.pos[1] + p.a[2] * a.pos[2] - p.d;
            const float db = p.a[0] * b.pos[0] + p.a[1] * b.pos[1] + p.a[2] * b.pos[2] - p.d;
            if (da <= 0) out.push_back(a);
            if ((da < 0) != (db < 0) && da != db) out.push_back(lerp(a, b, da / (da - db)));
        }
        poly->swap(out);
    }
}

struct Out {
    struct Group {
        m3g::Appearance* look;
        bool normals;
        int units;
        std::vector<PieceVertex> verts;
    };
    std::map<std::array<intptr_t, 2>, Group> groups;
    size_t tris = 0;

    void emit(const std::vector<PieceGroup>& set, const Affine& f, const std::vector<Plane>& planes) {
        std::vector<PieceVertex> poly;
        for (const PieceGroup& g : set) {
            Group* to = nullptr;
            for (const auto& tri : g.tris) {
                poly.clear();
                for (const PieceVertex& v : tri) {
                    PieceVertex w = v;
                    float n[3];
                    for (int r = 0; r < 3; r++) {
                        w.pos[r] = f.m[r][0] * v.pos[0] + f.m[r][1] * v.pos[1] + f.m[r][2] * v.pos[2] + f.t[r];
                        n[r] = f.n[r][0] * v.normal[0] + f.n[r][1] * v.normal[1] + f.n[r][2] * v.normal[2];
                    }
                    const float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
                    for (int r = 0; r < 3; r++) w.normal[r] = len > 0 ? n[r] / len : 0;
                    poly.push_back(w);
                }
                clip(&poly, planes);
                if (poly.size() < 3) continue;
                if (!to) {
                    Group& grp = groups[{intptr_t(g.look), intptr_t((g.normals ? 4 : 0) | g.units)}];
                    grp.look = g.look, grp.normals = g.normals, grp.units = g.units;
                    to = &grp;
                }
                for (size_t i = 1; i + 1 < poly.size(); i++) {
                    to->verts.insert(to->verts.end(), {poly[0], poly[i], poly[i + 1]});
                    tris++;
                }
            }
        }
    }
};

int cells(float lo, float hi, float step) { return std::max(1, int(ceilf((hi - lo) / step - 1e-4f))); }

Plane le(int axis, float v) {
    Plane p = {{0, 0, 0}, v};
    p.a[axis] = 1;
    return p;
}
Plane ge(int axis, float v) {
    Plane p = {{0, 0, 0}, -v};
    p.a[axis] = -1;
    return p;
}

void cover(const PieceSurface& ps, CollShape::ShapeType type, float x0, float y0, float z0, float x1, float y1, float z1, Out* out,
           int* white) {
    const port::platform::PiecePlanes& p = ps.planes;
    const float w = std::max(kMinStep, p.faceHi[0] - p.faceLo[0]), left = p.faceLo[0];
    float h = p.roof - p.faceLo[1], d = p.front - p.lo[2];
    if (h < kMinStep) h = std::max(kMinStep, p.faceHi[1] - p.faceLo[1]);
    if (d < kMinStep) d = std::max(kMinStep, p.hi[2] - p.lo[2]);
    const bool ramp = type == CollShape::SHAPE_RAMP_NEG_X || type == CollShape::SHAPE_RAMP_POS_X;
    const bool highAtMin = type == CollShape::SHAPE_RAMP_NEG_X;
    const float hx = highAtMin ? x0 : x1, lx = highAtMin ? x1 : x0;
    const int nx = cells(x0, x1, w), ny = cells(y0, y1, h), nz = cells(z0, z1, d);
    *white = ramp ? (kBack | kBottom | kFront | kSlope | (highAtMin ? kLeft : kRight)) : kAll & ~kSlope;
    const float slope = ramp ? (y0 - y1) / (lx - hx) : 0;
    Plane under = {{0, 0, 0}, 0};
    under.a[0] = -slope, under.a[1] = 1, under.d = y1 - slope * hx;
    auto xCell = [&](int i, std::vector<Plane>* planes) {
        planes->push_back(ge(0, x0 + float(i) * w));
        planes->push_back(le(0, std::min(x1, x0 + float(i + 1) * w)));
    };
    auto yCell = [&](int j, bool above, std::vector<Plane>* planes) {
        planes->push_back(le(1, j > 0 || !above ? y1 - float(j) * h : y1 + port::platform::kAboveTop));
        planes->push_back(ge(1, std::max(y0, y1 - float(j + 1) * h)));
    };
    auto zCell = [&](int k, std::vector<Plane>* planes) {
        planes->push_back(le(2, z1 - float(k) * d));
        planes->push_back(ge(2, std::max(z0, z1 - float(k + 1) * d)));
    };
    std::vector<Plane> planes;
    if (!ps.front.empty() && nx * ny <= kMaxTiles) {
        for (int i = 0; i < nx; i++)
            for (int j = 0; j < ny; j++) {
                Affine f;
                f.t[0] = x0 + float(i) * w - left, f.t[1] = y1 - float(j) * h - p.roof, f.t[2] = z1 - p.front;
                planes.clear();
                xCell(i, &planes);
                yCell(j, !ramp, &planes);
                if (ramp) planes.push_back(under);
                out->emit(ps.front, f, planes);
            }
        *white &= ~kFront;
    }
    if (!ps.roof.empty() && nx * nz <= kMaxTiles) {
        for (int i = 0; i < nx; i++)
            for (int k = 0; k < nz; k++) {
                Affine f;
                f.t[0] = x0 + float(i) * w - left, f.t[2] = z1 - float(k) * d - p.front;
                f.t[1] = y1 - p.roof;
                if (ramp) {
                    f.m[1][0] = slope;
                    f.t[1] += slope * (f.t[0] - hx);
                    f.n[0][1] = -slope;
                }
                planes.clear();
                xCell(i, &planes);
                zCell(k, &planes);
                out->emit(ps.roof, f, planes);
            }
        *white &= ~(ramp ? kSlope : kTop);
    }
    for (int side = 0; side < 2; side++) {
        const bool left = side == 0;
        const std::vector<PieceGroup>& set = left ? ps.left : ps.right;
        if (set.empty() || (ramp && left != highAtMin) || ny * nz > kMaxTiles) continue;
        for (int j = 0; j < ny; j++)
            for (int k = 0; k < nz; k++) {
                Affine f;
                f.t[0] = left ? x0 - p.left : x1 - p.right;
                f.t[1] = y1 - float(j) * h - p.roof, f.t[2] = z1 - float(k) * d - p.front;
                planes.clear();
                yCell(j, !ramp, &planes);
                zCell(k, &planes);
                out->emit(set, f, planes);
            }
        *white &= ~(left ? kLeft : kRight);
    }
}

int addSurfaceMeshes(const Out& out, m3g::Group* group) {
    int meshes = 0;
    for (const auto& kv : out.groups) {
        const Out::Group& g = kv.second;
        for (size_t first = 0; first < g.verts.size(); first += kMeshVertices) {
            const int n = int(std::min(g.verts.size() - first, size_t(kMeshVertices)));
            const PieceVertex* v = &g.verts[first];
            auto quantize = [&](int comps, auto get, float* scale, float* bias, std::vector<short>* q) {
                float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f}, extent = 0;
                for (int i = 0; i < n; i++)
                    for (int c = 0; c < comps; c++) lo[c] = std::min(lo[c], get(v[i], c)), hi[c] = std::max(hi[c], get(v[i], c));
                for (int c = 0; c < 3; c++) bias[c] = 0;
                for (int c = 0; c < comps; c++) bias[c] = (lo[c] + hi[c]) * 0.5f, extent = std::max(extent, hi[c] - lo[c]);
                *scale = extent > 0 ? extent / 65000.0f : 1.0f;
                q->resize(size_t(n) * size_t(comps));
                for (int i = 0; i < n; i++)
                    for (int c = 0; c < comps; c++) (*q)[size_t(i * comps + c)] = short(lrintf((get(v[i], c) - bias[c]) / *scale));
            };
            m3g::VertexBuffer* vb = new m3g::VertexBuffer();
            float scale, bias[3];
            std::vector<short> q;
            quantize(3, [](const PieceVertex& p, int c) { return p.pos[c]; }, &scale, bias, &q);
            m3g::VertexArray* positions = new m3g::VertexArray(n, 3, 2);
            positions->set(0, n, q.data());
            vb->setPositions(positions, scale, bias);
            std::vector<signed char> bytes(size_t(n) * 4);
            for (int i = 0; i < n; i++)
                for (int c = 0; c < 4; c++) bytes[size_t(i * 4 + c)] = (signed char)(unsigned char)lrintf(std::min(1.0f, std::max(0.0f, v[i].color[c])) * 255);
            m3g::VertexArray* colors = new m3g::VertexArray(n, 4, 1);
            colors->set(0, n, bytes.data());
            vb->setColors(colors);
            if (g.normals) {
                std::vector<signed char> nb(size_t(n) * 3);
                for (int i = 0; i < n; i++)
                    for (int c = 0; c < 3; c++) nb[size_t(i * 3 + c)] = (signed char)lrintf(v[i].normal[c] * 127);
                m3g::VertexArray* normals = new m3g::VertexArray(n, 3, 1);
                normals->set(0, n, nb.data());
                vb->setNormals(normals);
            }
            for (int u = 0; u < g.units; u++) {
                quantize(2, [u](const PieceVertex& p, int c) { return p.uv[u][c]; }, &scale, bias, &q);
                m3g::VertexArray* uv = new m3g::VertexArray(n, 2, 2);
                uv->set(0, n, q.data());
                vb->setTexCoords(u, uv, scale, bias);
            }
            midp::array<int> indices(n);
            for (int i = 0; i < n; i++) indices[i] = i;
            m3g::IndexBuffer* ib = new m3g::IndexBuffer(8, n / 3, indices);
            M3GAssets::addNode(group, new m3g::Mesh(vb, ib, g.look));
            meshes++;
        }
    }
    return meshes;
}

}

void medecomp_level_supports(MEdgeMap* map, m3g::Group* group, MapPalette* palette) {
    if (!map || !group) return;
    const bool all = medecomp_player_level_supports();
    const std::vector<int> shown = shownShapes();
    if (!all && shown.empty()) return;
    std::vector<bool> seen(size_t(map->m_shapes.m_length), false);
    std::map<int, PieceSurface> pieces;
    Builder b;
    Out out;
    int count = 0, covered = 0;
    for (int s = 0; s < map->m_numSections; s++)
        for (int i = map->m_sectionStart[s]; i < map->m_sectionEnd[s]; i++) {
            if (i < 0 || i >= map->m_shapes.m_length || seen[size_t(i)]) continue;
            seen[size_t(i)] = true;
            const int material = size_t(i) < shown.size() ? shown[size_t(i)] : -1;
            const int index = material < 0 ? -1 : port::platform::shapeMaterialIndex(material);
            const int surface = material < 0 ? 0 : port::platform::shapeSurface(material);
            if (!all && index != kShownMaterial && index != kDarkMaterial && surface == 0) continue;
            const Colors& k = index == kDarkMaterial ? kDark : kWhite;
            CollShape* shape = map->m_shapes[i];
            if (!shape) continue;
            const MathOrthoBox& box = shape->getBounds();
            const float x0 = box.m_min.x, y0 = box.m_min.y, z0 = box.m_min.z, x1 = box.m_max.x, y1 = box.m_max.y, z1 = box.m_max.z;
            if (x1 <= x0 || y1 <= y0) continue;
            int faces = kAll;
            if (surface != 0 && palette) {
                auto it = pieces.find(surface);
                if (it == pieces.end()) {
                    it = pieces.insert({surface, PieceSurface()}).first;
                    if (!port::platform::readPieceSurface(medecomp_piece_template(*palette, surface), &it->second))
                        fprintf(stderr, "medecomp: уровень игрока: куска поверхности %d нет в палитре\n", surface);
                }
                cover(it->second, shape->getShapeType(), x0, y0, z0, x1, y1, z1, &out, &faces);
                covered++;
            }
            switch (shape->getShapeType()) {
            case CollShape::SHAPE_RAMP_NEG_X: b.ramp(x0, y0, z0, x1, y1, z1, true, k, faces); break;
            case CollShape::SHAPE_RAMP_POS_X: b.ramp(x0, y0, z0, x1, y1, z1, false, k, faces); break;
            default: b.box(x0, y0, z0, x1, y1, z1, k, faces); break;
            }
            count++;
        }
    const int meshes = addSurfaceMeshes(out, group);
    if (covered)
        fprintf(stderr, "medecomp: уровень игрока: поверхности кусков — %d форм, %zu треугольников, %d сеток\n", covered, out.tris, meshes);
    const int n = int(b.pos.size() / 3);
    if (n == 0) return;
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (int i = 0; i < n; i++)
        for (int c = 0; c < 3; c++) lo[c] = std::min(lo[c], b.pos[size_t(i * 3 + c)]), hi[c] = std::max(hi[c], b.pos[size_t(i * 3 + c)]);
    float bias[3], extent = 0;
    for (int c = 0; c < 3; c++) bias[c] = (lo[c] + hi[c]) * 0.5f, extent = std::max(extent, hi[c] - lo[c]);
    const float scale = extent > 0 ? extent / 65000.0f : 1.0f;
    std::vector<short> q(size_t(n) * 3);
    for (int i = 0; i < n * 3; i++) q[size_t(i)] = short(lrintf((b.pos[size_t(i)] - bias[i % 3]) / scale));
    m3g::VertexBuffer* vb = new m3g::VertexBuffer();
    m3g::VertexArray* positions = new m3g::VertexArray(n, 3, 2);
    positions->set(0, n, q.data());
    vb->setPositions(positions, scale, bias);
    m3g::VertexArray* colors = new m3g::VertexArray(n, 4, 1);
    colors->set(0, n, reinterpret_cast<const signed char*>(b.col.data()));
    vb->setColors(colors);
    midp::array<int> indices(int(b.idx.size()));
    for (size_t i = 0; i < b.idx.size(); i++) indices[int(i)] = b.idx[i];
    m3g::IndexBuffer* ib = new m3g::IndexBuffer(8, int(b.idx.size() / 3), indices);
    m3g::Appearance* look = new m3g::Appearance();
    m3g::PolygonMode* mode = new m3g::PolygonMode();
    mode->setCulling(162);
    look->setPolygonMode(mode);
    m3g::Mesh* mesh = new m3g::Mesh(vb, ib, look);
    M3GAssets::addNode(group, mesh);
    fprintf(stderr, "medecomp: уровень игрока: %s — %d форм, %d вершин\n", all ? "опоры видны" : "белые формы", count, n);
}
