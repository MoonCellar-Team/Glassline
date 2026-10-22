#pragma once

#include <stdint.h>

#include <array>
#include <string>
#include <vector>

namespace port {
namespace editor {

using Fix = int32_t;
inline float toF(Fix v) { return v / 65536.0f; }
Fix toFix(float v);

struct Ref {
    int16_t layer = 0, column = 0, section = 0, node = 0;
};
constexpr int32_t kNodeMirror = 0x40000000;
struct Node {
    int32_t id = 0;
    bool mirror = false;
    Fix pos[3] = {};
    uint8_t shared = 1;
    int8_t visionFlag = 0;
    int8_t visionType = 0;
    Fix visionBox[4] = {};
};
struct Section {
    Fix bounds[6] = {};
    int8_t visionSlots = 0;
    std::vector<Node> nodes;
};
struct Column {
    Fix bounds[6] = {};
    std::vector<Section> sections;
};
struct Layer {
    std::vector<Column> columns;
};
struct Shape {
    Fix box[4] = {};
    int8_t type = 0;
    int32_t material = 0;
    int layer = 0;
};
struct PathPoint {
    float pos[3] = {};
    int32_t action = 0, speed = 0;
};
struct Object {
    int32_t type = 0;
    Fix pos[3] = {};
    int32_t path = -1;
    int8_t refFlag = 0;
    Ref ref;
};
struct Start {
    Fix pos[3] = {};
    int8_t facing = 1;
};
struct Checkpoint {
    Fix box[4] = {};
    Ref ref;
};
struct Sound {
    int32_t res = 0;
    Fix pos[3] = {};
};
struct Area {
    int32_t index = 0;
    Fix box[4] = {};
};
struct CameraStep {
    int32_t travel = 0, hold = 0;
    float from[3] = {}, at[3] = {};
};
struct CameraAnim {
    int8_t target = 0;
    std::vector<CameraStep> steps;
};

struct Map {
    int32_t palette = 0;
    int8_t unk004 = 0;
    int32_t modelSet = 0;
    Fix minX = 0, maxX = 0, sectionWidth = 0;
    int16_t numSections = 0;
    std::vector<std::pair<int16_t, int16_t>> strips;
    std::vector<Shape> shapes;
    std::vector<Layer> layers;
    std::vector<std::vector<PathPoint>> paths;
    std::vector<std::array<float, 6>> fallBoxes, painBoxes;
    std::vector<Object> objects;
    std::vector<Start> starts;
    std::vector<std::array<Fix, 4>> finishes;
    std::vector<Checkpoint> checkpoints;
    std::vector<Sound> ambientSounds, oneShots, musicCues;
    std::vector<std::array<Fix, 5>> zipLines;
    std::vector<Area> popups, cutsceneTriggers;
    std::vector<CameraAnim> cameraAnims;
};

const int kNumLayers = 4;
const int kBag = 3;
const int kSwing = 2;

bool readMap(const std::vector<unsigned char>& data, Map* map, std::string* error);
std::vector<unsigned char> writeMap(const Map& map);
void rebuildStrips(Map* map);
void growBounds(Map* map);
int selfCheck();

}
}
