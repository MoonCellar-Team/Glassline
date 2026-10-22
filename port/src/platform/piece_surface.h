#pragma once

#include <stdint.h>

#include <array>
#include <vector>

namespace m3g {
class Appearance;
class Node;
}

namespace port {
namespace platform {

inline int shapeMaterialIndex(int32_t material) { return material & 0xff; }
inline int shapeSurface(int32_t material) { return int(uint32_t(material) >> 8); }
inline int32_t withSurface(int32_t material, int piece) { return (material & 0xff) | int32_t(uint32_t(piece) << 8); }
constexpr int kMaxSurfacePiece = 0x7fffff;

constexpr float kFrontBand = 1.5f;
constexpr float kAboveTop = 2.5f;

struct PieceVertex {
    float pos[3], normal[3], color[4], uv[2][2];
};
struct PieceGroup {
    m3g::Appearance* look = nullptr;
    bool normals = false;
    int units = 0;
    std::vector<std::array<PieceVertex, 3>> tris;
};
struct PiecePlanes {
    float lo[3] = {}, hi[3] = {};
    float faceLo[2] = {}, faceHi[2] = {};
    float front = 0, roof = 0, left = 0, right = 0;
    bool hasFront = false, hasRoof = false, hasLeft = false, hasRight = false;
};
struct PieceSurface {
    PiecePlanes planes;
    std::vector<PieceGroup> front, roof, left, right;
};

bool readPieceSurface(m3g::Node* node, PieceSurface* out);

}
}
