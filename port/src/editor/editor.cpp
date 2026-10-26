#include "editor.h"
#include "../platform/touch_device.h"
#include "imgui_internal.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>

#include <algorithm>
#include <array>
#include <map>
#include <set>

#include "../ea/json.h"
#include "../gfx/png.h"
#include "../platform/launcher.h"
#include "../platform/levels.h"
#include "../platform/piece_surface.h"
#include "../platform/process.h"
#include "../platform/turn.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "imgui.h"
#include "game/MEdgeMap.h"
#include "game/ResourceManager.h"
#include "level.h"
#include "mcui.h"
#include "port/brand.h"
#include "port/files.h"
#include "port/platform.h"

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace port {
namespace editor {

namespace {

bool s_ru = true;
const char* L(const char* ru, const char* en) { return s_ru ? ru : en; }

#ifdef __EMSCRIPTEN__
float webEditorScale(float width, float height) {
    if (width < 900 || height < 620) return 0.7f;
    return std::max(1.0f, std::min(width / 1280.0f, height / 800.0f));
}
#endif

const float kSnap = 0.25f;
const float kLongPressSec = 0.5f;
#ifdef __ANDROID__
const bool kTouchScreen = true;
#else
const bool kTouchScreen = false;
#endif
const float kClosedSide = 0.8f;
const float kRoofed = 0.7f;
const float kDishX = 1.0f, kDishY = 0.5f;
const float kSwingHalf = 0.75f, kSwingHang = 1.6f;
const float kSurfaceStep = 0.25f;
const int kSurfaceTiles = 2000;
const float kRunFront = 1.0f;
const float kBackFront = -10.0f;
const float kFrontBack = 2.0f;
const float kRunBack = -9.0f;
const float kDepthNear = 6.0f, kDepthFar = -26.0f;
enum PaletteGroup { kGroupTops, kGroupCorners, kGroupSections, kGroupWalls, kGroupRoofs, kGroupFrames, kGroupProps, kGroupBack, kGroupOther, kGroupCount };
const float kPaletteWidth = 324.0f;
const float kPropSize = 3.0f;
const float kThinDepth = 1.0f;
const float kLowHeight = 4.0f;
const float kFrameCover = 0.35f;
std::string chapterTitle(const std::string& palette) {
    int a = 0, b = 0;
    if (sscanf(palette.c_str(), "chapter_%d_%d", &a, &b) == 2) return std::to_string(a) + "-" + std::to_string(b);
    if (sscanf(palette.c_str(), "chapter_%d", &a) == 1) return std::to_string(a);
    return palette;
}
const char* paletteGroupName(int g) {
    switch (g) {
    case kGroupTops: return L("Дома — верх с крышей", "Buildings — tops with a roof");
    case kGroupCorners: return L("Дома — углы и торцы", "Buildings — corners and ends");
    case kGroupSections: return L("Дома — секции и этажи", "Buildings — sections and floors");
    case kGroupWalls: return L("Плоские стены и вывески", "Flat walls and signs");
    case kGroupRoofs: return L("Крыши, карнизы, перила", "Roofs, ledges, railings");
    case kGroupFrames: return L("Каркасы, краны, леса", "Frames, cranes, scaffolds");
    case kGroupProps: return L("Мелочи", "Props");
    case kGroupBack: return L("Фон", "Background");
    default: return L("Прочее", "Other");
    }
}

enum class Kind { None, Shape, Node, Object, Start, Finish, Checkpoint, Sound, ZipStart, ZipEnd };

struct Sel {
    Kind kind = Kind::None;
    int index = 0;
    int list = 0;
    int layer = 0, column = 0, section = 0;
};

bool sameSel(const Sel& a, const Sel& b) {
    return a.kind == b.kind && a.index == b.index && a.list == b.list &&
           (a.kind != Kind::Node || (a.layer == b.layer && a.column == b.column && a.section == b.section));
}

struct Doc {
    std::string id, name, author, palette;
    long long version = 1;
    std::string origin = "chapter", description, serverJson;
    bool bags = false;
    int timeLimit = port::platform::kBagTimeLimitDefault;
    std::string supports;
    Map map;
    std::vector<Map> undo;
    bool open = false;
    std::string status;
};

std::string levelsDir() { return port::files::dataDir() + "/levels"; }

bool plainId(const std::string& id) {
    bool ok = !id.empty() && id.size() <= 64;
    for (char c : id) ok = ok && (isalnum((unsigned char)c) || c == '-' || c == '_');
    return ok;
}

std::vector<std::string> chapters() {
    std::vector<std::string> out, arenas;
    for (const std::string& name : port::files::list(port::files::bundle() + "/res")) {
        if (name.size() < 5 || name.compare(name.size() - 4, 4, ".bin") != 0) continue;
        const std::string base = name.substr(0, name.size() - 4);
        if (base.compare(0, 8, "chapter_") == 0) out.push_back(base);
        else if (port::platform::arenaBase(base)) arenas.push_back(base);
    }
    std::sort(out.begin(), out.end());
    std::sort(arenas.begin(), arenas.end());
    out.insert(out.end(), arenas.begin(), arenas.end());
    return out;
}

std::string baseTitle(const std::string& base) {
    if (!port::platform::arenaBase(base)) return std::string(L("Глава ", "Chapter ")) + chapterTitle(base);
    static const char* const names[] = {"Steps", "Joust", "Pinball", "Maze"};
    int n = 0;
    if (sscanf(base.c_str(), "multiplayer_%d", &n) == 1 && n >= 1 && n <= 4) return std::string(L("Арена ", "Arena ")) + names[n - 1];
    return base;
}

std::vector<std::string> levelIds() {
    std::vector<std::string> out;
    for (const std::string& id : port::files::list(levelsDir()))
        if (plainId(id) && port::files::stat(levelsDir() + "/" + id + "/level.json").exists) out.push_back(id);
    std::sort(out.begin(), out.end());
    return out;
}

bool openChapter(const std::string& chapter, Map* map, std::string* error) {
    std::vector<unsigned char> data;
    if (!port::files::read(port::files::bundle() + "/res/" + chapter + ".bin", &data))
        return *error = chapter + L(": нет в копии игры", ": not in the game copy"), false;
    return readMap(data, map, error);
}

bool openLevel(const std::string& id, Doc* doc, std::string* error) {
    std::string dir = levelsDir() + "/" + id;
    std::vector<unsigned char> json, data;
    port::online::Json info;
    if (!port::files::read(dir + "/level.json", &json) ||
        !port::online::parseJson(std::string(json.begin(), json.end()), &info) || !port::files::read(dir + "/level.bin", &data))
        return *error = dir + L(": нет level.json или level.bin", ": no level.json or level.bin"), false;
    Doc d;
    if (!readMap(data, &d.map, error)) return false;
    d.id = id;
    d.name = info["name"].str();
    d.author = info["author"].str();
    d.palette = info["palette"].str();
    d.version = info["version"].integer(1);
    d.origin = info["origin"].str("chapter");
    d.description = info["description"].str();
    port::platform::LevelMode mode = port::platform::readLevelMode(id);
    d.bags = mode.bags;
    if (mode.bags) d.timeLimit = mode.timeLimit;
    d.supports = info["supports"].str();
    const port::online::Json& server = info["server"];
    if (server.type == port::online::Json::Object)
        d.serverJson = "{\"id\": " + port::online::jsonString(server["id"].str()) +
                       ", \"version\": " + std::to_string(server["version"].integer()) +
                       ", \"board\": " + port::online::jsonString(server["board"].str()) + "}";
    d.open = true;
    *doc = std::move(d);
    return true;
}

bool writeFile(const std::string& path, const std::string& data) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    return fclose(f) == 0 && ok;
}

std::string save(Doc* doc) {
    rebuildStrips(&doc->map);
    growBounds(&doc->map);
    std::vector<unsigned char> data = writeMap(doc->map);
    std::string dir = levelsDir() + "/" + doc->id;
    mkdir(levelsDir().c_str(), 0755);
    mkdir(dir.c_str(), 0755);
    std::string json = "{\n  \"name\": " + port::online::jsonString(doc->name) +
                       ",\n  \"author\": " + port::online::jsonString(doc->author) +
                       ",\n  \"palette\": " + port::online::jsonString(doc->palette) +
                       ",\n  \"version\": " + std::to_string(doc->version) +
                       ",\n  \"origin\": " + port::online::jsonString(doc->origin) +
                       ",\n  \"description\": " + port::online::jsonString(doc->description) +
                       (doc->bags ? ",\n  \"mode\": \"bags\",\n  \"timeLimit\": " + std::to_string(doc->timeLimit)
                                  : std::string(",\n  \"mode\": \"race\"")) +
                       (doc->supports.empty() ? std::string() : ",\n  \"supports\": " + port::online::jsonString(doc->supports)) +
                       (doc->serverJson.empty() ? std::string() : ",\n  \"server\": " + doc->serverJson) + "\n}\n";
    if (!writeFile(dir + "/level.bin", std::string(data.begin(), data.end())) || !writeFile(dir + "/level.json", json))
        return doc->status = L("не записать ", "cannot write ") + dir;
    std::string check = port::platform::checkPlayerMap(data, doc->bags);
    doc->status = check.empty() ? std::string(L("сохранено: ", "saved: ")) + dir
                                : std::string(L("сохранено, но в игру не пойдёт: ", "saved, but the game will not take it: ")) + check;
    return check;
}

std::string selfExe(const char* argv0) {
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, sizeof buf);
    return n ? std::string(buf, n) : std::string(argv0);
#else
    std::string self = port::platform::process::selfPath();
    return self.empty() ? std::string(argv0) : self;
#endif
}

}

long long launch(const std::vector<std::string>& args, const char* argv0) {
    std::string exe = selfExe(argv0), game = port::files::gameSource();
    std::vector<std::string> all = {exe, "--game", game};
    all.insert(all.end(), args.begin(), args.end());
    std::vector<char*> argv;
    for (auto& a : all) argv.push_back(&a[0]);
    argv.push_back(nullptr);
#ifdef _WIN32
    intptr_t pid = _spawnv(_P_NOWAIT, exe.c_str(), argv.data());
    return pid == -1 ? -1 : (long long)pid;
#else
    pid_t pid;
    return posix_spawn(&pid, exe.c_str(), nullptr, nullptr, argv.data(), environ) == 0 ? (long long)pid : -1;
#endif
}

namespace {

#ifdef MEDECOMP_ONE_PROCESS
std::string s_testLevel;
bool s_service = false;
std::vector<std::string> s_serviceEnv;
std::string s_serviceLevel;
struct Resume {
    bool valid = false;
    std::string what;
    float cx = 0, cy = 0, zoom = 0;
    int hasArea = 0;
    float area[4] = {};
} s_resume;
std::string resumePath() { return port::files::dataDir() + "/cache/editor-resume"; }
#endif

long long startTest(Doc* doc, const char* argv0) {
    if (!save(doc).empty()) return -1;
#ifdef MEDECOMP_ONE_PROCESS
    s_testLevel = doc->id;
    return 0;
#endif
    long long pid = launch({"--level", doc->id, "--test"}, argv0);
    doc->status = pid < 0 ? std::string(L("игра не запустилась", "the game did not start"))
                          : std::string(L("игра запущена: уровень ", "game started: level ")) + doc->id;
    return pid;
}

void pushUndo(Doc* doc) {
    doc->undo.push_back(doc->map);
    if (doc->undo.size() > 200) doc->undo.erase(doc->undo.begin());
}

struct View {
    float cx = 0, cy = 0, zoom = 4;
    ImVec2 origin, size;
    ImVec2 toScreen(float x, float y) const {
        return ImVec2(origin.x + size.x / 2 + (x - cx) * zoom, origin.y + size.y / 2 - (y - cy) * zoom);
    }
    void toWorld(ImVec2 p, float* x, float* y) const {
        *x = cx + (p.x - origin.x - size.x / 2) / zoom;
        *y = cy - (p.y - origin.y - size.y / 2) / zoom;
    }
};

Fix* posOf(Map& m, const Sel& s) {
    switch (s.kind) {
    case Kind::Shape: return m.shapes[size_t(s.index)].box;
    case Kind::Node: return m.layers[size_t(s.layer)].columns[size_t(s.column)].sections[size_t(s.section)].nodes[size_t(s.index)].pos;
    case Kind::Object: return m.objects[size_t(s.index)].pos;
    case Kind::Start: return m.starts[size_t(s.index)].pos;
    case Kind::Finish: return m.finishes[size_t(s.index)].data();
    case Kind::Checkpoint: return m.checkpoints[size_t(s.index)].box;
    case Kind::Sound: {
        auto& list = s.list == 0 ? m.ambientSounds : s.list == 1 ? m.oneShots : m.musicCues;
        return list[size_t(s.index)].pos;
    }
    case Kind::ZipStart: return m.zipLines[size_t(s.index)].data();
    case Kind::ZipEnd: return m.zipLines[size_t(s.index)].data() + 2;
    default: return nullptr;
    }
}

Node* refNode(Map& m, const Ref& r) {
    auto in = [](int i, size_t n) { return i >= 0 && size_t(i) < n; };
    if (!in(r.layer, m.layers.size()) || !in(r.column, m.layers[size_t(r.layer)].columns.size())) return nullptr;
    auto& sections = m.layers[size_t(r.layer)].columns[size_t(r.column)].sections;
    if (!in(r.section, sections.size()) || !in(r.node, sections[size_t(r.section)].nodes.size())) return nullptr;
    return &sections[size_t(r.section)].nodes[size_t(r.node)];
}
bool sameRef(const Ref& a, const Ref& b) { return a.layer == b.layer && a.column == b.column && a.section == b.section && a.node == b.node; }
int refCount(const Map& m, const Ref& r) {
    int n = 0;
    for (const auto& c : m.checkpoints) n += sameRef(c.ref, r);
    for (const auto& o : m.objects) n += o.refFlag == 1 && sameRef(o.ref, r);
    return n;
}
Sel refSel(const Ref& r) {
    Sel s;
    s.kind = Kind::Node, s.layer = r.layer, s.column = r.column, s.section = r.section, s.index = r.node;
    return s;
}

void moveSel(Map& m, const Sel& s, float dx, float dy, bool dish = true) {
    Fix* p = posOf(m, s);
    if (!p) return;
    Fix fx = toFix(dx), fy = toFix(dy);
    p[0] += fx;
    p[1] += fy;
    if (s.kind == Kind::Shape || s.kind == Kind::Checkpoint) {
        p[2] += fx;
        p[3] += fy;
    }
    if (s.kind == Kind::Checkpoint && dish) {
        const Ref& r = m.checkpoints[size_t(s.index)].ref;
        Node* n = refNode(m, r);
        if (n && refCount(m, r) == 1) {
            n->pos[0] += fx, n->pos[1] += fy;
            n->visionBox[0] += fx, n->visionBox[1] += fy, n->visionBox[2] += fx, n->visionBox[3] += fy;
        }
    }
}

enum Edge { EdgeLeft = 1, EdgeRight = 2, EdgeBottom = 4, EdgeTop = 8 };

bool resizable(const Sel& s) { return s.kind == Kind::Shape || s.kind == Kind::Checkpoint || s.kind == Kind::Finish; }

bool selBox(Map& m, const Sel& s, float* b) {
    Fix* p = resizable(s) ? posOf(m, s) : nullptr;
    if (!p) return false;
    for (int i = 0; i < 4; i++) b[i] = toF(p[i]);
    if (s.kind == Kind::Finish) b[2] += b[0], b[3] += b[1];
    return true;
}

void setSelBox(Map& m, const Sel& s, const float* b) {
    Fix* p = posOf(m, s);
    for (int i = 0; i < 4; i++) p[i] = toFix(b[i]);
    if (s.kind == Kind::Finish) p[2] = toFix(b[2] - b[0]), p[3] = toFix(b[3] - b[1]);
}

bool selValid(const Map& m, const Sel& s) {
    auto in = [](int i, size_t n) { return i >= 0 && size_t(i) < n; };
    switch (s.kind) {
    case Kind::Shape: return in(s.index, m.shapes.size());
    case Kind::Node:
        return in(s.layer, m.layers.size()) && in(s.column, m.layers[size_t(s.layer)].columns.size()) &&
               in(s.section, m.layers[size_t(s.layer)].columns[size_t(s.column)].sections.size()) &&
               in(s.index, m.layers[size_t(s.layer)].columns[size_t(s.column)].sections[size_t(s.section)].nodes.size());
    case Kind::Object: return in(s.index, m.objects.size());
    case Kind::Start: return in(s.index, m.starts.size());
    case Kind::Finish: return in(s.index, m.finishes.size());
    case Kind::Checkpoint: return in(s.index, m.checkpoints.size());
    case Kind::Sound: return in(s.index, (s.list == 0 ? m.ambientSounds : s.list == 1 ? m.oneShots : m.musicCues).size());
    case Kind::ZipStart:
    case Kind::ZipEnd: return in(s.index, m.zipLines.size());
    default: return false;
    }
}

void eraseNode(Map& m, int layer, int column, int section, int index) {
    auto fix = [&](Ref& r) {
        if (r.layer == layer && r.column == column && r.section == section && r.node > index) r.node--;
    };
    for (auto& o : m.objects)
        if (o.refFlag == 1) fix(o.ref);
    for (auto& c : m.checkpoints) fix(c.ref);
    auto& nodes = m.layers[size_t(layer)].columns[size_t(column)].sections[size_t(section)].nodes;
    nodes.erase(nodes.begin() + index);
}

bool eraseSel(Map& m, const Sel& s, const char** why, bool withDish = false) {
    if (!selValid(m, s)) return false;
    if ((s.kind == Kind::Start && m.starts.size() <= 1) || (s.kind == Kind::Finish && m.finishes.size() <= 1)) {
        *why = L("последний старт и финиш не удаляются", "the last start and finish stay");
        return false;
    }
    auto erase = [&](auto& v) { v.erase(v.begin() + s.index); };
    switch (s.kind) {
    case Kind::Shape: erase(m.shapes); break;
    case Kind::Node: {
        Section& sec = m.layers[size_t(s.layer)].columns[size_t(s.column)].sections[size_t(s.section)];
        const Ref here = {int16_t(s.layer), int16_t(s.column), int16_t(s.section), int16_t(s.index)};
        const bool lit = sec.nodes[size_t(s.index)].visionFlag == 1;
        if (sec.nodes.size() <= 1 || (lit && refCount(m, here) > 0)) {
            *why = L("этот кусок не удаляется: это тарелка чекпоинта или последний кусок в секции",
                     "this piece stays: it is a checkpoint's dish or the last piece in its section");
            return false;
        }
        eraseNode(m, s.layer, s.column, s.section, s.index);
        if (lit && sec.visionSlots > 0) sec.visionSlots--;
        break;
    }
    case Kind::Object: erase(m.objects); break;
    case Kind::Start: erase(m.starts); break;
    case Kind::Finish: erase(m.finishes); break;
    case Kind::Checkpoint: {
        const Ref dish = m.checkpoints[size_t(s.index)].ref;
        erase(m.checkpoints);
        if (withDish && refCount(m, dish) == 0) {
            Node* n = refNode(m, dish);
            const char* ignore = nullptr;
            if (n && n->visionFlag == 1) eraseSel(m, refSel(dish), &ignore);
        }
        break;
    }
    case Kind::Sound: erase(s.list == 0 ? m.ambientSounds : s.list == 1 ? m.oneShots : m.musicCues); break;
    case Kind::ZipStart:
    case Kind::ZipEnd: erase(m.zipLines); break;
    default: break;
    }
    return true;
}

void deleteSel(Doc* doc, Sel* s) {
    if (!selValid(doc->map, *s)) return;
    const char* why = nullptr;
    pushUndo(doc);
    if (!eraseSel(doc->map, *s, &why, true)) {
        doc->undo.pop_back();
        if (why) doc->status = why;
        return;
    }
    *s = Sel();
}

void deleteGroup(Doc* doc, std::vector<Sel> all) {
    for (Sel& s : all)
        if (s.kind == Kind::ZipEnd) s.kind = Kind::ZipStart;
    auto key = [](const Sel& s) { return std::array<int, 6>{int(s.kind), s.list, s.layer, s.column, s.section, s.index}; };
    std::sort(all.begin(), all.end(), [&](const Sel& a, const Sel& b) { return key(a) > key(b); });
    all.erase(std::unique(all.begin(), all.end(), sameSel), all.end());
    pushUndo(doc);
    int removed = 0, kept = 0;
    const char* why = nullptr;
    for (const Sel& s : all) {
        if (eraseSel(doc->map, s, &why)) removed++;
        else kept++;
    }
    if (!removed) doc->undo.pop_back();
    if (kept) doc->status = why ? why : L("не всё удалилось", "not everything was deleted");
}

bool sectionAt(const Map& m, float x, Sel* out) {
    float best = 1e30f;
    bool found = false;
    for (size_t l = 0; l < m.layers.size(); l++)
        for (size_t c = 0; c < m.layers[l].columns.size(); c++)
            for (size_t s = 0; s < m.layers[l].columns[c].sections.size(); s++) {
                const Fix* b = m.layers[l].columns[c].sections[s].bounds;
                float d = x < toF(b[0]) ? toF(b[0]) - x : x > toF(b[3]) ? x - toF(b[3]) : 0;
                if (d < best) {
                    best = d;
                    out->layer = int(l), out->column = int(c), out->section = int(s);
                    found = true;
                }
            }
    return found;
}

bool giveDish(Map& m, int i, const Node& source, float x, float y, Fix z) {
    Sel at;
    if (!sectionAt(m, x, &at)) return false;
    Section& sec = m.layers[size_t(at.layer)].columns[size_t(at.column)].sections[size_t(at.section)];
    if (sec.nodes.size() >= 127 || sec.visionSlots >= 127) return false;
    Node n = source;
    n.mirror = false, n.visionFlag = 1, n.shared = 0;
    if (n.visionType == 0) n.visionType = 2;
    const Fix dx = toFix(x) - source.pos[0], dy = toFix(y) - source.pos[1];
    n.pos[0] = toFix(x), n.pos[1] = toFix(y), n.pos[2] = z;
    n.visionBox[0] += dx, n.visionBox[1] += dy, n.visionBox[2] += dx, n.visionBox[3] += dy;
    auto shift = [&](Ref& r) {
        if (r.layer == at.layer && r.column == at.column && r.section == at.section) r.node++;
    };
    for (auto& c : m.checkpoints) shift(c.ref);
    for (auto& o : m.objects)
        if (o.refFlag == 1) shift(o.ref);
    sec.nodes.insert(sec.nodes.begin(), n);
    sec.visionSlots++;
    m.checkpoints[size_t(i)].ref = {int16_t(at.layer), int16_t(at.column), int16_t(at.section), 0};
    return true;
}

ImU32 layerColor(int layer, int alpha) {
    static const ImU32 rgb[] = {IM_COL32(220, 220, 225, 0), IM_COL32(110, 200, 120, 0), IM_COL32(240, 160, 60, 0),
                                IM_COL32(170, 120, 230, 0)};
    return (rgb[std::max(0, std::min(3, layer))] & 0x00ffffffu) | ImU32(alpha) << 24;
}

const char* objectName(int type) {
    switch (type) {
    case 0: return L("голуби", "pigeons");
    case 1: return L("вертолёт", "chopper");
    case kSwing: return L("турник", "swing bar");
    case kBag: return L("сумка", "bag");
    case 4: return L("полиция", "police");
    case 5: return L("соперник", "rival");
    case 6: return L("свет самолёта", "plane light");
    case 7: return L("соперник 2", "rival 2");
    case 8: return L("полиция со щитом", "riot police");
    default: return "?";
    }
}

enum class Tool { Select, Hand, Area, Block, RampL, RampR, Bag, Checkpoint, Zip, Music, Piece, Swing };

const int kBagModel = 0x10f, kBagModelAlt = 0x10e;

ImU32 tok(unsigned token, float alpha = 1.0f) { return mcui::Packed(token, alpha); }
const ImU32 kAccentLight = IM_COL32(180, 166, 255, 255);
const ImU32 kDanger = IM_COL32(255, 138, 140, 255);

struct Icons {
    mcui::Icon select, hand, area, block, rampL, rampR, bag, flag, zip, music, piece, close, undo, trash, up, down, info, swing;
};

const Icons& icons() {
    static const Icons set = [] {
        Icons i;
        i.select = mcui::ParseIcon(R"(<path d="M5 3 L19 12 L12.5 13.5 L9.5 20 Z"/>)");
        i.area = mcui::ParseIcon(R"(<path d="M4 9 V4 H9 M15 4 H20 V9 M20 15 V20 H15 M9 20 H4 V15"/>)");
        i.block = mcui::ParseIcon(R"(<path d="M4 7 H20 V17 H4 Z"/>)");
        i.rampL = mcui::ParseIcon(R"(<path d="M4 18 H20 L4 7 Z"/>)");
        i.rampR = mcui::ParseIcon(R"(<path d="M4 18 H20 V7 Z"/>)");
        i.bag = mcui::ParseIcon(R"(<path d="M4 10 H20 V18.5 A1.5 1.5 0 0 1 18.5 20 H5.5 A1.5 1.5 0 0 1 4 18.5 Z"/>)"
                                R"(<path d="M4 10 L6.5 14 H17.5 L20 10"/><path d="M7 10 C7 2.5 17 2.5 17 10"/><path d="M11 14 V16.5 H13 V14"/>)");
        i.hand = mcui::ParseIcon(R"(<path d="M8 13 V5.5 A1.5 1.5 0 0 1 11 5.5 V11 M11 10 V4.5 A1.5 1.5 0 0 1 14 4.5 V11)"
                                 R"( M14 10.5 V5.5 A1.5 1.5 0 0 1 17 5.5 V12 M17 9.5 A1.5 1.5 0 0 1 20 9.5 V15 A6 6 0 0 1 14 21)"
                                 R"( H12.5 A6 6 0 0 1 7.6 18.4 L4.6 13.8 A1.5 1.5 0 0 1 7.2 12.3 L8 13.5"/>)");
        i.flag = mcui::ParseIcon(R"(<path d="M6 21 V4"/><path d="M6 5 H17 L14.5 8.5 L17 12 H6"/>)");
        i.zip = mcui::ParseIcon(R"(<path d="M4 6.5 L20 15.5"/><path d="M1.5 6 A1.5 1.5 0 1 0 4.5 6 A1.5 1.5 0 1 0 1.5 6"/>)"
                                R"(<path d="M19.5 16 A1.5 1.5 0 1 0 22.5 16 A1.5 1.5 0 1 0 19.5 16"/>)");
        i.music = mcui::ParseIcon(R"(<path d="M9 18 V6 L19 4 V16"/><path d="M5 18 A2 2 0 1 0 9 18 A2 2 0 1 0 5 18"/>)"
                                  R"(<path d="M15 16 A2 2 0 1 0 19 16 A2 2 0 1 0 15 16"/>)");
        i.piece = mcui::ParseIcon(R"(<path d="M12 3 L20 7.5 V16.5 L12 21 L4 16.5 V7.5 Z"/><path d="M4 7.5 L12 12 L20 7.5 M12 12 V21"/>)");
        i.close = mcui::ParseIcon(R"(<path d="M6 6 L18 18 M18 6 L6 18"/>)");
        i.undo = mcui::ParseIcon(R"(<path d="M9 14 L4 9 L9 4"/><path d="M4 9 H14 A6 6 0 0 1 14 21 H10"/>)");
        i.trash = mcui::ParseIcon(R"(<path d="M5 7 H19 M9 7 V4 H15 V7 M7 7 L8 20 H16 L17 7"/>)");
        i.up = mcui::ParseIcon(R"(<path d="M6 15 L12 9 L18 15"/>)");
        i.down = mcui::ParseIcon(R"(<path d="M6 9 L12 15 L18 9"/>)");
        i.swing = mcui::ParseIcon(R"(<path d="M3 6 H21"/><path d="M5 6 V20 M19 6 V20"/><path d="M10 6 V11 M14 6 V11"/>)"
                                  R"(<path d="M10.5 13 A1.5 1.5 0 1 0 13.5 13 A1.5 1.5 0 1 0 10.5 13"/>)");
        i.info = mcui::ParseIcon(R"(<path d="M3 12 A9 9 0 1 0 21 12 A9 9 0 1 0 3 12"/><path d="M12 11 V16.5"/><path d="M12 7.6 V7.8"/>)");
        return i;
    }();
    return set;
}

ImU32 faded(ImU32 c) { return ImGui::GetColorU32(ImGui::ColorConvertU32ToFloat4(c)); }

extern "C" int medecomp_map_table_size(int table);

const int kSoundEventsResource = 223;

std::string soundFile(int index) {
    static std::vector<int> eventFile;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        std::vector<unsigned char> d;
        if (port::files::read(port::files::bundle() + "/res/" + ResourceManager::RESOURCE_FILENAMES_LIST[kSoundEventsResource], &d)) {
            size_t p = 0;
            auto u8 = [&]() { return p < d.size() ? int(d[p++]) : -1; };
            auto s16 = [&]() {
                int a = u8(), b = u8();
                return a < 0 || b < 0 ? -1 : int(int16_t(uint16_t((a << 8) | b)));
            };
            const int groups = u8();
            p += size_t(std::max(0, groups));
            const int events = s16();
            for (int i = 0; i < events && p + 9 <= d.size(); i++) {
                const int file = s16();
                p += 7;
                eventFile.push_back(file >= 0 ? ResourceManager::SOUND_DATA_SETS[file] : -1);
            }
        }
    }
    if (index < 0 || index >= medecomp_map_table_size(0)) return std::string();
    const int event = MEdgeMap::RES_LOOKUP_ARRAY[index];
    if (event < 0 || event >= int(eventFile.size()) || eventFile[size_t(event)] < 0) return std::string();
    return ResourceManager::RESOURCE_FILENAMES_LIST[eventFile[size_t(event)]];
}

std::string soundName(int index) {
    std::string f = soundFile(index);
    if (f.empty()) return "#" + std::to_string(index);
    if (f.rfind('.') != std::string::npos) f.erase(f.rfind('.'));
    if (f.compare(0, 4, "bgm_") == 0) f.erase(0, 4);
    std::string out;
    for (size_t i = 0; i < f.size(); i++) {
        char c = f[i] == '_' ? ' ' : f[i];
        if (i > 0 && isupper((unsigned char)c) && islower((unsigned char)f[i - 1])) out += ' ';
        out += c;
    }
    return out;
}

std::vector<int> soundChoices(bool music) {
    std::vector<int> out;
    std::vector<std::string> seen;
    for (int i = 0; i < medecomp_map_table_size(0); i++) {
        std::string f = soundFile(i);
        if (f.empty() || (f.compare(0, 4, "bgm_") == 0) != music) continue;
        if (std::find(seen.begin(), seen.end(), f) != seen.end()) continue;
        seen.push_back(f);
        out.push_back(i);
    }
    std::sort(out.begin(), out.end(), [](int a, int b) { return soundName(a) < soundName(b); });
    return out;
}

class Editor {
public:
    Editor(const char* argv0, float scale, SDL_Renderer* renderer) : m_argv0(argv0), m_scale(scale), m_renderer(renderer) {
        m_touchSeen = port::platform::defaultTouchInput();
#ifdef __ANDROID__
        m_levelOpen = false;
#elif defined(__EMSCRIPTEN__)
        if (scale < 1.0f) m_levelOpen = false;
#endif
    }

    void uiScale(float scale) {
        if (scale == m_scale) return;
        if (scale < 1.0f && m_scale >= 1.0f) m_levelOpen = false;
        view.zoom *= scale / m_scale;
        m_scale = scale;
    }

    Doc doc;
    Sel sel;
    View view;
    bool showShapes = true, showGraphics = true, showObjects = true;
    bool showDepth = true;
    long long testPid = -1;

    void fit() {
        Map& m = doc.map;
        view.cx = (toF(m.minX) + toF(m.maxX)) / 2;
        float lo = 1e30f, hi = -1e30f;
        for (auto& s : m.shapes) lo = std::min(lo, toF(s.box[1])), hi = std::max(hi, toF(s.box[3]));
        view.cy = lo < hi ? (lo + hi) / 2 : 0;
        if (!m.starts.empty()) view.cx = toF(m.starts[0].pos[0]) + 30, view.cy = toF(m.starts[0].pos[1]);
        view.zoom = 8 * m_scale;
    }
#ifdef MEDECOMP_ONE_PROCESS
    void resume() {
        if (!s_resume.valid) return;
        s_resume.valid = false;
        if (s_resume.zoom > 0) view.cx = s_resume.cx, view.cy = s_resume.cy, view.zoom = s_resume.zoom;
        m_hasArea = s_resume.hasArea != 0;
        for (int i = 0; i < 4; i++) m_area[i] = s_resume.area[i];
        if (s_resume.what == "map") {
            m_mapTex = loadTexture(cacheDir() + "/map.png", &m_mapSize);
            m_mapOpen = m_mapTex != nullptr;
            if (!m_mapTex) doc.status = L("кадр области не вышел (игра не загрузила уровень?)", "no area shot (did the game load the level?)");
        } else if (s_resume.what == "pieces" && !m_piecesEnv.empty()) {
            doc.status = L("картинки кусков не вышли — подробности в журнале", "piece previews failed — see the log");
        }
    }
#endif

    void addShape(int type, int layer) { addShape(type, layer, roundf(view.cx), roundf(view.cy)); }
    void addShape(int type, int layer, float x, float y) {
        pushUndo(&doc);
        Shape s;
        s.type = int8_t(type);
        s.layer = layer;
        s.box[0] = toFix(x), s.box[1] = toFix(y);
        s.box[2] = s.box[0] + toFix(4), s.box[3] = s.box[1] + toFix(type ? 2 : 1);
        doc.map.shapes.push_back(s);
        select(Kind::Shape, int(doc.map.shapes.size()) - 1);
    }
    void addObject(int type, float x, float y) {
        pushUndo(&doc);
        Object o;
        o.type = type;
        o.pos[0] = toFix(x), o.pos[1] = toFix(y);
        doc.map.objects.push_back(o);
        select(Kind::Object, int(doc.map.objects.size()) - 1);
    }
    const Checkpoint* checkpointModel(const Node** dish) {
        Map& from = !doc.map.checkpoints.empty() ? doc.map : m_paletteMap;
        if (from.checkpoints.empty()) return nullptr;
        *dish = refNode(from, from.checkpoints[0].ref);
        return *dish ? &from.checkpoints[0] : nullptr;
    }
    void addCheckpoint(float x, float y) {
        const Node* model = nullptr;
        const Checkpoint* first = checkpointModel(&model);
        if (!first) {
            doc.status = L("чекпоинт не из чего поставить: их нет ни в уровне, ни в главе-основе",
                           "nothing to make a checkpoint from: neither the level nor its base chapter has one");
            return;
        }
        if (doc.map.checkpoints.empty()) {
            const Node source = *model;
            const Checkpoint from = *first;
            pushUndo(&doc);
            Checkpoint c;
            c.box[0] = toFix(x), c.box[1] = toFix(y);
            c.box[2] = c.box[0] + (from.box[2] - from.box[0]), c.box[3] = c.box[1] + (from.box[3] - from.box[1]);
            doc.map.checkpoints.push_back(c);
            sectionFor(x + kDishX, y + kDishY);
            if (!giveDish(doc.map, 0, source, x + kDishX, y + kDishY, source.pos[2])) {
                undo();
                doc.status = L("в секции нет места для тарелки", "no room for the dish in the section");
                return;
            }
            select(Kind::Checkpoint, 0);
            return;
        }
        pushUndo(&doc);
        Checkpoint c = doc.map.checkpoints[0];
        float w = toF(c.box[2]) - toF(c.box[0]), h = toF(c.box[3]) - toF(c.box[1]);
        c.box[0] = toFix(x), c.box[1] = toFix(y);
        c.box[2] = c.box[0] + toFix(w), c.box[3] = c.box[1] + toFix(h);
        doc.map.checkpoints.push_back(c);
        const int i = int(doc.map.checkpoints.size()) - 1;
        if (const Node* dish = refNode(doc.map, c.ref)) {
            const Node source = *dish;
            if (!giveDish(doc.map, i, source, x + kDishX, y + kDishY, source.pos[2]))
                doc.status = L("тарелка — общая с первым чекпоинтом: в секции нет места", "the dish is shared with the first checkpoint: the section is full");
        }
        select(Kind::Checkpoint, i);
    }
    void ownDish(int i) {
        Map& m = doc.map;
        Checkpoint& c = m.checkpoints[size_t(i)];
        const Node* now = refNode(m, c.ref);
        if (!now) return;
        const Node source = *now;
        const float x0 = toF(c.box[0]), y0 = toF(c.box[1]), x1 = toF(c.box[2]), y1 = toF(c.box[3]);
        Sel hand;
        for (size_t l = 0; l < m.layers.size() && hand.kind == Kind::None; l++)
            for (size_t k = 0; k < m.layers[l].columns.size() && hand.kind == Kind::None; k++)
                for (size_t q = 0; q < m.layers[l].columns[k].sections.size() && hand.kind == Kind::None; q++) {
                    const auto& nodes = m.layers[l].columns[k].sections[q].nodes;
                    for (size_t n = 0; n < nodes.size() && hand.kind == Kind::None; n++) {
                        const float ox = toF(nodes[n].pos[0]), oy = toF(nodes[n].pos[1]);
                        if (nodes[n].id == source.id && nodes[n].visionFlag != 1 && ox >= x0 - 3 && ox <= x1 + 3 && oy >= y0 - 3 && oy <= y1)
                            hand = refSel({int16_t(l), int16_t(k), int16_t(q), int16_t(n)});
                    }
                }
        float x = x0 + kDishX, y = y0 + kDishY;
        Fix z = source.pos[2];
        pushUndo(&doc);
        if (hand.kind == Kind::Node) {
            Section& sec = m.layers[size_t(hand.layer)].columns[size_t(hand.column)].sections[size_t(hand.section)];
            Node& o = sec.nodes[size_t(hand.index)];
            x = toF(o.pos[0]), y = toF(o.pos[1]), z = o.pos[2];
            if (sec.nodes.size() == 1) {
                const Fix dx = o.pos[0] - source.pos[0], dy = o.pos[1] - source.pos[1];
                o.visionFlag = 1, o.visionType = source.visionType ? source.visionType : 2;
                for (int k = 0; k < 4; k++) o.visionBox[k] = source.visionBox[k] + (k % 2 ? dy : dx);
                sec.visionSlots = std::max<int8_t>(sec.visionSlots, 1);
                c.ref = {int16_t(hand.layer), int16_t(hand.column), int16_t(hand.section), 0};
                return;
            }
            eraseNode(m, hand.layer, hand.column, hand.section, hand.index);
        }
        if (!giveDish(m, i, source, x, y, z)) {
            undo();
            doc.status = L("в секции нет места для тарелки", "no room for the dish in the section");
        }
    }
    void addZip(float x, float y) {
        pushUndo(&doc);
        std::array<Fix, 5> z = {toFix(x), toFix(y + 4), toFix(x + 16), toFix(y), toFix(1)};
        doc.map.zipLines.push_back(z);
        select(Kind::ZipStart, int(doc.map.zipLines.size()) - 1);
    }
    void addCue(float x, float y) {
        pushUndo(&doc);
        Sound s;
        s.res = doc.map.musicCues.empty() ? 6 : doc.map.musicCues[0].res;
        s.pos[0] = toFix(x), s.pos[1] = toFix(y);
        doc.map.musicCues.push_back(s);
        select(Kind::Sound, int(doc.map.musicCues.size()) - 1, 2);
    }
    void addPiece(int id, float x, float y) {
        pushUndo(&doc);
        sel = placeNode(id, x, y);
    }
    Sel sectionFor(float x, float y) {
        Sel at;
        if (!sectionAt(doc.map, x, &at)) {
            Column column;
            column.sections.push_back(Section());
            const float box[6] = {x - 8, y - 8, -20, x + 8, y + 8, 4};
            for (int i = 0; i < 6; i++) column.bounds[i] = column.sections[0].bounds[i] = toFix(box[i]);
            Layer layer;
            layer.columns.push_back(column);
            doc.map.layers.push_back(layer);
            at.layer = int(doc.map.layers.size()) - 1, at.column = 0, at.section = 0;
        }
        return at;
    }
    Sel placeNode(int id, float x, float y) {
        Sel at = sectionFor(x, y);
        Node n;
        n.id = id;
        n.shared = 0;
        n.pos[0] = toFix(x), n.pos[1] = toFix(y);
        auto box = m_pieceBox.find(id);
        if (box != m_pieceBox.end()) n.pos[2] = toFix(kRunFront - box->second[5]);
        auto& nodes = doc.map.layers[size_t(at.layer)].columns[size_t(at.column)].sections[size_t(at.section)].nodes;
        nodes.push_back(n);
        at.kind = Kind::Node;
        at.index = int(nodes.size()) - 1;
        return at;
    }
    void coverShape(int id) {
        Map& m = doc.map;
        if (sel.kind != Kind::Shape || !selValid(m, sel) || !m_paletteIds.count(id)) return;
        auto box = m_pieceBox.find(id);
        if (box == m_pieceBox.end()) {
            doc.status = L("у куска ещё нет размера — превью готовятся", "the piece has no size yet — previews are being made");
            return;
        }
        const auto& b = box->second;
        const float w = b[3] - b[0];
        const Shape& s = m.shapes[size_t(sel.index)];
        const float ax = toF(s.box[0]), bx = toF(s.box[2]), top = toF(s.box[3]);
        if (w < kSnap) {
            doc.status = L("кусок слишком узкий", "the piece is too narrow");
            return;
        }
        const int caps[2] = {capFor(id, 0), capFor(id, 1)};
        float capsWidth = 0;
        for (int c : caps)
            if (c >= 0) capsWidth += m_pieceBox[c][3] - m_pieceBox[c][0];
        const int count = std::max(1, int(ceilf((bx - ax - capsWidth) / w - 0.01f)));
        if (count > 400) {
            doc.status = L("коробка слишком длинная для этого куска (больше 400 копий)", "the block is too long for this piece (over 400 copies)");
            return;
        }
        pushUndo(&doc);
        float x = ax;
        auto put = [&](int piece) {
            const auto& pb = m_pieceBox[piece];
            placeNode(piece, x - pb[0], top - pb[4]);
            x += pb[3] - pb[0];
        };
        if (caps[0] >= 0) put(caps[0]);
        for (int i = 0; i < count; i++) put(id);
        if (caps[1] >= 0) put(caps[1]);
        m_piece = id;
        char text[160];
        int len = snprintf(text, sizeof text, L("покрыто: %d × кусок %d", "covered: %d × piece %d"), count, id);
        for (int side = 0; side < 2; side++)
            if (caps[side] >= 0 && len > 0 && len < int(sizeof text))
                len += snprintf(text + len, sizeof text - size_t(len), side ? L(", торец справа %d", ", right end %d") : L(", торец слева %d", ", left end %d"),
                                caps[side]);
        auto own = m_pieceSides.find(id);
        if (own != m_pieceSides.end() && own->second[2] < kRoofed && len > 0 && len < int(sizeof text))
            snprintf(text + len, sizeof text - size_t(len), "%s", L("; крыши у куска нет — сверху видно насквозь", "; the piece has no roof — see-through from above"));
        doc.status = text;
    }
    static std::map<int, std::pair<int, uint8_t>> nodeIds(const Map& m) {
        std::map<int, std::pair<int, uint8_t>> ids;
        for (auto& l : m.layers)
            for (auto& c : l.columns)
                for (auto& s : c.sections)
                    for (auto& n : s.nodes) ids[n.id] = {ids[n.id].first + 1, n.shared};
        return ids;
    }
    void loadPalette() {
        Map chapter;
        std::string error;
        bool opened = openChapter(doc.palette, &chapter, &error);
        m_paletteIds = opened ? nodeIds(chapter) : nodeIds(doc.map);
        m_paletteMap = opened ? chapter : doc.map;
        startPieces(m_paletteMap);
    }
    void select(Kind kind, int index, int list = 0) {
        sel = Sel();
        sel.kind = kind;
        sel.index = index;
        sel.list = list;
    }
    std::vector<Sel> selection() const {
        std::vector<Sel> all;
        if (selValid(doc.map, sel)) all.push_back(sel);
        for (const Sel& g : m_group)
            if (selValid(doc.map, g)) all.push_back(g);
        return all;
    }
    bool inSelection(const Sel& s) const {
        if (s.kind == Kind::None) return false;
        if (sameSel(s, sel)) return true;
        for (const Sel& g : m_group)
            if (sameSel(g, s)) return true;
        return false;
    }
    void toggleSelected(const Sel& s) {
        if (s.kind == Kind::None) return;
        if (sameSel(s, sel)) {
            sel = Sel();
            if (!m_group.empty()) sel = m_group.back(), m_group.pop_back();
        } else {
            auto it = std::find_if(m_group.begin(), m_group.end(), [&](const Sel& g) { return sameSel(g, s); });
            if (it != m_group.end()) m_group.erase(it);
            else {
                if (selValid(doc.map, sel)) m_group.push_back(sel);
                sel = s;
            }
        }
        m_groupFor = sel;
    }
    void keepGroup() {
        if (!sameSel(sel, m_groupFor) || !selValid(doc.map, sel)) m_group.clear();
        m_group.erase(std::remove_if(m_group.begin(), m_group.end(),
                                     [&](const Sel& g) { return !selValid(doc.map, g) || sameSel(g, sel); }),
                      m_group.end());
        m_groupFor = sel;
    }
    void forgetShapes() {
        if (sel.kind == Kind::Shape) sel = Sel();
        m_group.erase(std::remove_if(m_group.begin(), m_group.end(), [](const Sel& g) { return g.kind == Kind::Shape; }), m_group.end());
        if (sel.kind == Kind::None && !m_group.empty()) sel = m_group.back(), m_group.pop_back();
        m_groupFor = sel;
    }
    void selectInArea() {
        Map& m = doc.map;
        float a[4];
        area(a);
        auto in = [&](float x, float y) { return x >= a[0] && x <= a[2] && y >= a[1] && y <= a[3]; };
        std::vector<Sel> all;
        auto add = [&](Kind k, int i, int list = 0) {
            Sel s;
            s.kind = k, s.index = i, s.list = list;
            all.push_back(s);
        };
        if (showShapes)
            for (size_t i = 0; i < m.shapes.size(); i++) {
                const Fix* b = m.shapes[i].box;
                if (in(toF(b[0]), toF(b[1])) && in(toF(b[2]), toF(b[3]))) add(Kind::Shape, int(i));
            }
        if (showGraphics)
            for (size_t l = 0; l < m.layers.size(); l++)
                for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                    for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++) {
                        const auto& nodes = m.layers[l].columns[c].sections[k].nodes;
                        for (size_t n = 0; n < nodes.size(); n++) {
                            const float x = toF(nodes[n].pos[0]), y = toF(nodes[n].pos[1]);
                            std::array<float, 6> b;
                            if (nodeBox(nodes[n], &b) ? !(in(x + b[0], y + b[1]) && in(x + b[3], y + b[4])) : !in(x, y)) continue;
                            Sel s;
                            s.kind = Kind::Node, s.index = int(n), s.layer = int(l), s.column = int(c), s.section = int(k);
                            all.push_back(s);
                        }
                    }
        if (showObjects) {
            for (size_t i = 0; i < m.starts.size(); i++)
                if (in(toF(m.starts[i].pos[0]), toF(m.starts[i].pos[1]))) add(Kind::Start, int(i));
            for (size_t i = 0; i < m.objects.size(); i++)
                if (in(toF(m.objects[i].pos[0]), toF(m.objects[i].pos[1]))) add(Kind::Object, int(i));
            const std::vector<Sound>* lists[] = {&m.ambientSounds, &m.oneShots, &m.musicCues};
            for (int l = 0; l < 3; l++)
                for (size_t i = 0; i < lists[l]->size(); i++)
                    if (in(toF((*lists[l])[i].pos[0]), toF((*lists[l])[i].pos[1]))) add(Kind::Sound, int(i), l);
            for (size_t i = 0; i < m.zipLines.size(); i++)
                if (in(toF(m.zipLines[i][0]), toF(m.zipLines[i][1])) && in(toF(m.zipLines[i][2]), toF(m.zipLines[i][3])))
                    add(Kind::ZipStart, int(i)), add(Kind::ZipEnd, int(i));
            for (size_t i = 0; i < m.finishes.size(); i++) {
                const auto& f = m.finishes[i];
                if (in(toF(f[0]), toF(f[1])) && in(toF(f[0]) + toF(f[2]), toF(f[1]) + toF(f[3]))) add(Kind::Finish, int(i));
            }
            for (size_t i = 0; i < m.checkpoints.size(); i++) {
                const Fix* b = m.checkpoints[i].box;
                if (in(toF(b[0]), toF(b[1])) && in(toF(b[2]), toF(b[3]))) add(Kind::Checkpoint, int(i));
            }
        }
        m_hasArea = false;
        if (all.empty()) {
            doc.status = L("в области ничего нет целиком", "nothing is wholly inside the area");
            return;
        }
        sel = all.front();
        m_group.assign(all.begin() + 1, all.end());
        m_groupFor = sel;
    }
    std::string groupSummary() const {
        int shapes = 0, nodes = 0, other = 0;
        for (const Sel& s : selection()) (s.kind == Kind::Shape ? shapes : s.kind == Kind::Node ? nodes : other)++;
        std::string out;
        auto part = [&](int n, const char* what) {
            if (!n) return;
            if (!out.empty()) out += " · ";
            out += std::string(what) + ": " + std::to_string(n);
        };
        part(shapes, L("коробки и рампы", "blocks and ramps"));
        part(nodes, L("куски", "pieces"));
        part(other, L("объекты", "objects"));
        return out;
    }

    bool compactMobile() const {
#ifdef __ANDROID__
        return true;
#elif defined(__EMSCRIPTEN__)
        return ImGui::GetIO().DisplaySize.x < 900 || ImGui::GetIO().DisplaySize.y < 620;
#else
        return false;
#endif
    }

    void frame(bool* quit) {
        ImGuiIO& io = ImGui::GetIO();
        float safeBottom = 0;
#ifdef __EMSCRIPTEN__
        safeBottom = float(MAIN_THREAD_EM_ASM_DOUBLE({
            const area = document.getElementById("safe-area");
            return area ? area.getBoundingClientRect().height : 0;
        }));
#endif
        const bool editing = doc.open;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, std::max(1.0f, io.DisplaySize.y - safeBottom)));
        if (editing) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##editor", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         (editing ? ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse : 0));
        if (editing) ImGui::PopStyleVar();
        if (!editing) {
            if (compactMobile()) port::platform::panelTouchScroll(m_scale);
            levelList(quit);
        }
        else editorView();
        ImGui::End();
        if (editing && doc.open) overlays();
        pollPreviews();
        mapWindow();
    }

    void finger(const SDL_TouchFingerEvent& f, int w, int h) {
        m_touchSeen = true;
        if (f.type == SDL_FINGERUP) m_fingers.erase(f.fingerId);
        else m_fingers[f.fingerId] = ImVec2(f.x * float(w), f.y * float(h));
        if (f.type == SDL_FINGERUP && m_fingers.empty()) ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

private:
    const char* m_argv0;
    float m_scale;
    char m_newId[64] = "my-level";
    int m_chapter = 0;
    std::string m_message;
    std::map<int, std::pair<int, uint8_t>> m_paletteIds;
    bool m_dragging = false, m_dragUndo = false;
    float m_dragX = 0, m_dragY = 0;
    std::vector<Sel> m_group;
    Sel m_groupFor, m_groupClick;
    Sel m_cycleTo, m_stackSel;
    int m_stackCount = 0, m_stackPos = 0;
    int m_resize = 0;
    float m_addX = 0, m_addY = 0;
    bool m_demoPickBag = false;
    bool m_areaDrag = false, m_hasArea = false;
    float m_area[4] = {};
    Tool m_tool = Tool::Select;
    int m_piece = -1;
    bool m_levelOpen = true;
    float m_levelBottom = 0, m_actionsBottom = 0, m_inspectorH = 0, m_viewBarW = 0, m_viewBarH = 0;
    ImVec2 m_inspectorMin, m_inspectorMax;
    std::string m_openInfo;
    int m_openInfoFrame = 0;
    std::string m_statusClosed;
    std::map<SDL_FingerID, ImVec2> m_fingers;
    bool m_touchSeen = kTouchScreen, m_pinch = false, m_pinchHold = false;
    bool m_longPress = false;
    float m_pinchX = 0, m_pinchY = 0, m_pinchZoom = 1, m_pinchDist = 1;
    SDL_Renderer* m_renderer;
    long m_piecesChild = 0, m_mapChild = 0;
    std::string m_piecesDir;
    std::map<int, std::pair<SDL_Texture*, ImVec2>> m_pieceTex;
    std::map<int, std::array<float, 6>> m_pieceBox;
    std::map<int, std::array<float, 3>> m_pieceSides;
    std::map<int, std::pair<SDL_Texture*, ImVec2>> m_frontTex;
    std::map<int, std::pair<SDL_Texture*, ImVec2>> m_viewTex;
    std::map<int, float> m_pieceCover;
    std::map<int, int> m_pieceGroup;
    struct PieceFace {
        bool front = false;
        float roof = 0, lo[2] = {}, hi[2] = {};
    };
    std::map<int, PieceFace> m_pieceFace;
    bool m_openSurface = false;
    bool m_openCover = false;
    int m_progressDone = 0, m_progressTotal = 0;
    Uint32 m_progressAt = 0;
    bool m_depthDrag = false, m_depthUndo = false;
    float m_depthStartY = 0, m_depthStartZ = 0;
    ImVec2 m_depthSelAt = ImVec2(-1, -1);
    ImVec2 m_surfaceAt;
    std::set<int> m_flatTop;
    Map m_paletteMap;
    SDL_Texture* m_mapTex = nullptr;
    ImVec2 m_mapSize;
    bool m_mapOpen = false;
#ifdef MEDECOMP_ONE_PROCESS
    std::vector<std::string> m_piecesEnv;

    bool writeResume(const char* what) {
        mkdir(cacheDir().c_str(), 0755);
        FILE* f = fopen(resumePath().c_str(), "wb");
        if (!f) {
            doc.status = L("не записать файл возврата в редактор", "cannot write the editor return file");
            return false;
        }
        fprintf(f, "%s\n%s %.4f %.4f %.4f %d %.4f %.4f %.4f %.4f\n", doc.id.c_str(), what, view.cx, view.cy, view.zoom,
                m_hasArea ? 1 : 0, m_area[0], m_area[1], m_area[2], m_area[3]);
        fclose(f);
        return true;
    }

    void requestService(const std::vector<std::string>& env, const std::string& level, const char* what) {
        if (!save(&doc).empty()) return;
        forgetShapes();
        if (!writeResume(what)) return;
        s_serviceEnv = env;
        s_serviceLevel = level;
        s_service = true;
    }
#endif

    void levelList(bool* quit) {
        const float s = m_scale;
        const float w = std::min(640 * s, ImGui::GetContentRegionAvail().x);
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowWidth() - w) / 2, 40 * s));
        mcui::BeginPanel("##levels", ImVec2(w, 0));
        ImGui::PushFont(mcui::GetFonts().bold, 22.0f);
        ImGui::TextUnformatted(L("Редактор уровней", "Level editor"));
        ImGui::PopFont();
        caption(levelsDir().c_str());
        mcui::SectionTitle(L("Свои уровни", "My levels"));
        std::vector<std::string> ids = levelIds();
        if (ids.empty()) ImGui::TextDisabled("%s", L("пока нет", "none yet"));
        const float open = ImGui::CalcTextSize(L("Открыть", "Open")).x + 2 * ImGui::GetStyle().FramePadding.x;
        for (const std::string& id : ids) {
            ImGui::PushID(id.c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(id.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - open);
            if (mcui::Button(L("Открыть", "Open"))) {
                std::string error;
                if (openLevel(id, &doc, &error)) fit(), sel = Sel(), loadPalette();
                else m_message = error;
            }
            ImGui::PopID();
        }
        mcui::SectionTitle(L("Новый уровень", "New level"));
        std::vector<std::string> names = chapters();
        if (names.empty()) {
            ImGui::TextDisabled("%s", L("в копии игры нет глав", "no chapters in the game copy"));
        } else {
            m_chapter = std::min(m_chapter, int(names.size()) - 1);
            fieldLabel(L("id — латиница, цифры, - и _", "id — latin, digits, - and _"));
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputText("##id", m_newId, sizeof m_newId);
            fieldLabel(L("Основа — глава или арена: её палитра и модели", "Base — a chapter or an arena: its palette and models"));
            ImGui::SetNextItemWidth(-FLT_MIN);
            const std::string chosen = baseTitle(names[size_t(m_chapter)]) + "  (" + names[size_t(m_chapter)] + ")";
            if (ImGui::BeginCombo("##chapter", chosen.c_str())) {
                for (size_t i = 0; i < names.size(); i++) {
                    const std::string item = baseTitle(names[i]) + "  (" + names[i] + ")";
                    if (ImGui::Selectable(item.c_str(), int(i) == m_chapter)) m_chapter = int(i);
                }
                ImGui::EndCombo();
            }
            const bool arena = port::platform::arenaBase(names[size_t(m_chapter)]);
            if (mcui::Button(L("Создать с нуля", "Create from scratch"), mcui::ButtonKind::Accent))
                create(m_newId, names[size_t(m_chapter)], true);
            ImGui::SameLine();
            if (mcui::Button(arena ? L("Копия арены", "Arena copy") : L("Копия главы", "Chapter copy"))) create(m_newId, names[size_t(m_chapter)]);
            caption(arena ? L("Арена BAG HUNT — только сбор сумок. С нуля — можно выложить в сеть; копия арены несёт разметку EA и "
                              "остаётся у себя.",
                              "A BAG HUNT arena takes bag collection only. From scratch can be uploaded; an arena copy carries EA's "
                              "layout and stays here.")
                          : L("С нуля — можно выложить в сеть; копия главы несёт разметку EA и остаётся у себя.",
                              "From scratch can be uploaded; a chapter copy carries EA's layout and stays here."));
        }
        if (!m_message.empty()) ImGui::TextWrapped("%s", m_message.c_str());
        mcui::EndPanel();
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - w) / 2);
        if (mcui::Button(L("Выход", "Quit"), mcui::ButtonKind::Ghost)) *quit = true;
    }

public:
    bool create(const std::string& id, const std::string& chapter, bool empty = false) {
        std::string error;
        if (!plainId(id)) return m_message = L("id — латиница, цифры, - и _", "id: latin, digits, - and _"), false;
        if (port::files::stat(levelsDir() + "/" + id).exists) return m_message = id + L(" уже есть", " already exists"), false;
        Doc d;
        if (!openChapter(chapter, &d.map, &error)) return m_message = error, false;
        m_paletteIds = nodeIds(d.map);
        m_paletteMap = d.map;
        if (empty) {
            Map blank;
            blank.palette = d.map.palette;
            blank.unk004 = d.map.unk004;
            blank.modelSet = d.map.modelSet;
            blank.sectionWidth = d.map.sectionWidth;
            blank.minX = toFix(-100);
            blank.maxX = toFix(300);
            Shape floor;
            const float box[4] = {-90, -1, 250, 0};
            for (int i = 0; i < 4; i++) floor.box[i] = toFix(box[i]);
            blank.shapes.push_back(floor);
            blank.starts.push_back(Start());
            if (port::platform::arenaBase(chapter)) {
                Object bag;
                bag.type = kBag;
                bag.pos[0] = toFix(20), bag.pos[1] = toFix(1);
                blank.objects.push_back(bag);
            } else {
                blank.finishes.push_back({toFix(200), toFix(0), toFix(10), toFix(6)});
            }
            d.map = blank;
            d.origin = "empty";
        }
        if (port::platform::arenaBase(chapter)) d.bags = true;
        d.id = id;
        d.name = id;
        d.palette = chapter;
        d.open = true;
        doc = std::move(d);
        sel = Sel();
        fit();
        save(&doc);
        startPieces(doc.map);
        return true;
    }

    void doSave() {
        save(&doc);
        forgetShapes();
    }
    void test() {
#ifdef MEDECOMP_ONE_PROCESS
        if (!writeResume("test")) return;
#endif
        testPid = startTest(&doc, m_argv0);
        forgetShapes();
    }
    int nodeCount() const {
        int n = 0;
        for (auto& l : doc.map.layers)
            for (auto& c : l.columns)
                for (auto& s : c.sections) n += int(s.nodes.size());
        return n;
    }
    bool demoFill() {
        Map& m = doc.map;
        if (m.starts.empty()) return false;
        float sx = toF(m.starts[0].pos[0]), best = 1e30f;
        for (size_t l = 0; l < m.layers.size(); l++)
            for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++) {
                    auto& nodes = m.layers[l].columns[c].sections[k].nodes;
                    for (size_t n = 0; n < nodes.size(); n++)
                        if (m_pieceBox.count(nodes[n].id) && fabsf(toF(nodes[n].pos[0]) - sx) < best) {
                            best = fabsf(toF(nodes[n].pos[0]) - sx);
                            sel = Sel(), sel.kind = Kind::Node, sel.layer = int(l), sel.column = int(c), sel.section = int(k), sel.index = int(n);
                        }
                }
        if (sel.kind != Kind::Node) return false;
        const Node& n = m.layers[size_t(sel.layer)].columns[size_t(sel.column)].sections[size_t(sel.section)].nodes[size_t(sel.index)];
        const auto& b = m_pieceBox[n.id];
        float w = b[3] - b[0], x = toF(n.pos[0]) + b[3] + 2, y = toF(n.pos[1]) + b[1];
        m_hasArea = true;
        m_area[0] = x, m_area[1] = y, m_area[2] = x + 4 * w, m_area[3] = y + (b[4] - b[1]);
        view.cx = x + 2 * w, view.cy = y + (b[4] - b[1]) / 2, view.zoom = 3 * m_scale;
        int before = nodeCount();
        fillArea();
        int filled = nodeCount();
        doc.map = doc.undo.back(), doc.undo.pop_back();
        int undone = nodeCount();
        fillArea();
        int refilled = nodeCount();
        clearArea();
        int cleared = nodeCount();
        doc.map = doc.undo.back(), doc.undo.pop_back();
        fprintf(stderr, "medecomp: редактор: демо L8 — кусок %d (%.1f × %.1f), узлов: %d, заполнено %d, после отмены %d, "
                "снова %d, удалено в области -> %d, отмена -> %d\n", n.id, w, b[4] - b[1], before, filled, undone, refilled,
                cleared, nodeCount());
        sel.kind = Kind::None;
        return filled > before && undone == before && cleared < refilled && nodeCount() == refilled;
    }
    bool demoPreviewArea() {
        previewArea();
        return m_mapChild != 0;
    }
    bool demoCover(int piece) {
        Map& m = doc.map;
        if (m.starts.empty() || m.shapes.empty()) return false;
        float floorTop = toF(m.shapes[0].box[3]);
        for (auto& s : m.shapes) floorTop = std::max(floorTop, toF(s.box[3]));
        const float x = roundf(toF(m.starts[0].pos[0])) + 8, y = floorTop + 2;
        addShape(0, 0, x, y);
        int best = piece > 0 && m_pieceBox.count(piece) ? piece : -1;
        for (auto& kv : m_paletteIds) {
            auto b = m_pieceBox.find(kv.first);
            if (piece > 0 || b == m_pieceBox.end()) continue;
            const float w = b->second[3] - b->second[0], h = b->second[4] - b->second[1];
            if (w < 1 || w > 8 || h > 6) continue;
            if (best < 0 || kv.second.first > m_paletteIds[best].first) best = kv.first;
        }
        if (best < 0) return false;
        const float len = piece > 0 ? 3 * (m_pieceBox[best][3] - m_pieceBox[best][0]) : 4;
        m.shapes.back().box[2] = toFix(x + len);
        const int before = nodeCount();
        coverShape(best);
        const int after = nodeCount();
        m_hasArea = true;
        if (piece > 0) {
            m_area[0] = x - 0.3f * len, m_area[1] = y - 0.3f * len, m_area[2] = x + 1.3f * len, m_area[3] = y + 0.3f * len;
        } else {
            m_area[0] = x - 10, m_area[1] = y - 4, m_area[2] = x + 14, m_area[3] = y + 8;
        }
        view.cx = x + len / 2, view.cy = y + 2;
        fprintf(stderr, "medecomp: редактор: демо L20 — коробка %.2f %.2f (4 × 1), кусок %d (%.2f × %.2f): узлов %d -> %d; %s\n", x, y, best,
                m_pieceBox[best][3] - m_pieceBox[best][0], m_pieceBox[best][4] - m_pieceBox[best][1], before, after, doc.status.c_str());
        previewArea();
        return after > before && m_mapChild != 0;
    }
    bool demoSurface(int piece) {
        Map& m = doc.map;
        if (m.starts.empty() || m.shapes.empty() || view.size.x <= 0) return false;
        int best = piece > 0 ? piece : -1;
        for (auto& kv : m_paletteIds) {
            auto f = m_pieceFace.find(kv.first);
            if (piece > 0 || f == m_pieceFace.end() || !f->second.front) continue;
            const float w = f->second.hi[0] - f->second.lo[0], h = f->second.roof - f->second.lo[1], above = f->second.hi[1] - f->second.roof;
            if (w < 6 || w > 14 || h < 6 || h > 16 || above > 3) continue;
            if (best < 0 || kv.second.first > m_paletteIds[best].first) best = kv.first;
        }
        if (best < 0 || !m_pieceFace.count(best)) return false;
        float floorTop = toF(m.shapes[0].box[3]);
        for (auto& s : m.shapes) floorTop = std::max(floorTop, toF(s.box[3]));
        const float x = roundf(toF(m.starts[0].pos[0])) + 6, y = floorTop;
        addShape(2, 0, x, y);
        m.shapes.back().box[2] = toFix(x + 8), m.shapes.back().box[3] = toFix(y + 6);
        m.shapes.back().material = port::platform::withSurface(m.shapes.back().material, best);
        addShape(0, 0, x + 8, y);
        m.shapes.back().box[2] = toFix(x + 26), m.shapes.back().box[3] = toFix(y + 10);
        m.shapes.back().material = port::platform::withSurface(m.shapes.back().material, best);
        view.zoom = view.size.x * 0.5f / 30, view.cx = x + 13, view.cy = y + 5;
        const PieceFace& f = m_pieceFace[best];
        fprintf(stderr, "medecomp: редактор: демо L41 — рампа и коробка у x %.2f, кусок %d (лицо %.2f × %.2f, крыша %.2f над низом)\n", x,
                best, f.hi[0] - f.lo[0], f.hi[1] - f.lo[1], f.roof - f.lo[1]);
        doSave();
        for (size_t i = 0; i < m.shapes.size(); i++)
            if (m.shapes[i].type == 0 && m.shapes[i].box[0] == toFix(x + 8) && m.shapes[i].box[2] == toFix(x + 26)) select(Kind::Shape, int(i));
        if (getenv("MEDECOMP_EDITOR_DEMO_L41_PICK")) m_openSurface = true, m_surfaceAt = view.toScreen(x + 27, y + 10);
        return true;
    }
    bool demoWaiting() const { return m_piecesChild || m_mapChild; }
    int demoProgress() const { return m_piecesChild ? m_progressDone : -1; }
    bool demoDepth(const char* preset) {
        Map& m = doc.map;
        if (m.starts.empty()) return false;
        const float sx = toF(m.starts[0].pos[0]), sy = toF(m.starts[0].pos[1]);
        Sel best;
        float bestD = 1e30f;
        for (size_t l = 0; l < m.layers.size(); l++)
            for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++) {
                    auto& nodes = m.layers[l].columns[c].sections[k].nodes;
                    for (size_t i = 0; i < nodes.size(); i++) {
                        if (depthOf(nodes[i]) != Depth::Run) continue;
                        const float d = fabsf(toF(nodes[i].pos[0]) - sx) + fabsf(toF(nodes[i].pos[1]) - sy);
                        if (d < bestD) best.kind = Kind::Node, best.index = int(i), best.layer = int(l), best.column = int(c), best.section = int(k), bestD = d;
                    }
                }
        if (best.kind != Kind::Node) return false;
        sel = best;
        Node& n = m.layers[size_t(best.layer)].columns[size_t(best.column)].sections[size_t(best.section)].nodes[size_t(best.index)];
        const auto& b = m_pieceBox[n.id];
        view.zoom = view.size.x * 0.25f / std::max(1.0f, b[3] - b[0]);
        view.cx = toF(n.pos[0]) + (b[0] + b[3]) / 2, view.cy = toF(n.pos[1]) + (b[1] + b[4]) / 2;
        const float before = toF(n.pos[2]);
        const std::string p = preset ? preset : "";
        if (p == "back" || p == "run" || p == "front") {
            pushUndo(&doc);
            n.pos[2] = toFix(p == "back" ? kBackFront - b[5] : p == "front" ? kFrontBack - b[2] : kRunFront - b[5]);
        }
        fprintf(stderr, "medecomp: редактор: демо L46 — кусок %d (z %.2f…%.2f по коробке), z %.2f -> %.2f\n", n.id, b[2], b[5], before,
                toF(n.pos[2]));
        return true;
    }
    ImVec2 demoDepthPoint() const { return m_depthSelAt; }
    float demoDepthZ() const {
        const Map& m = doc.map;
        if (sel.kind != Kind::Node || !selValid(m, sel)) return 0;
        return toF(m.layers[size_t(sel.layer)].columns[size_t(sel.column)].sections[size_t(sel.section)].nodes[size_t(sel.index)].pos[2]);
    }
    bool demoMirror() {
        Map& m = doc.map;
        if (m.starts.empty()) return false;
        const float sx = toF(m.starts[0].pos[0]);
        float best = -1;
        for (size_t l = 0; l < m.layers.size(); l++)
            for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++) {
                    auto& nodes = m.layers[l].columns[c].sections[k].nodes;
                    for (size_t i = 0; i < nodes.size(); i++) {
                        std::array<float, 6> b;
                        if (!nodeBox(nodes[i], &b) || depthOf(nodes[i]) != Depth::Run || fabsf(toF(nodes[i].pos[0]) - sx) > 60) continue;
                        if (fabsf(b[0] + b[3]) > best)
                            best = fabsf(b[0] + b[3]), sel = Sel(), sel.kind = Kind::Node, sel.index = int(i), sel.layer = int(l),
                            sel.column = int(c), sel.section = int(k);
                    }
                }
        if (sel.kind != Kind::Node || !selValid(m, sel)) return false;
        Node& n = m.layers[size_t(sel.layer)].columns[size_t(sel.column)].sections[size_t(sel.section)].nodes[size_t(sel.index)];
        std::array<float, 6> before, after;
        if (!nodeBox(n, &before)) return false;
        const float x = toF(n.pos[0]);
        pushUndo(&doc);
        n.pos[0] = toFix(x + before[0] + before[3]);
        n.mirror = !n.mirror;
        nodeBox(n, &after);
        fprintf(stderr, "medecomp: редактор: демо L48 — кусок %d отражён: x %.2f -> %.2f, коробка по x %.2f…%.2f -> %.2f…%.2f\n", n.id, x,
                toF(n.pos[0]), x + before[0], x + before[3], toF(n.pos[0]) + after[0], toF(n.pos[0]) + after[3]);
        const Sel keep = sel;
        save(&doc);
        sel = keep;
        return true;
    }
    void demoGroups() {
        demoPieceTool();
        int count[kGroupCount] = {};
        for (auto& kv : m_pieceGroup) count[kv.second]++;
        std::string text;
        for (int g = 0; g < kGroupCount; g++) text += std::string(g ? ", " : "") + paletteGroupName(g) + " " + std::to_string(count[g]);
        fprintf(stderr, "medecomp: редактор: демо L47 — %zu кусков: %s\n", m_pieceGroup.size(), text.c_str());
    }
    void demoFit(const char* area) {
        float a[4];
        if (area && sscanf(area, "%f,%f,%f,%f", &a[0], &a[1], &a[2], &a[3]) == 4 && a[2] > a[0] && a[3] > a[1]) {
            view.zoom = std::min(view.size.x / (a[2] - a[0]), view.size.y / (a[3] - a[1]));
            view.cx = (a[0] + a[2]) / 2, view.cy = (a[1] + a[3]) / 2;
        } else {
            fit();
        }
    }
    bool demoSurfacePopup(bool cover) {
        if (!demoSelectBlock()) return false;
        if (cover) m_openCover = true;
        else m_openSurface = true, m_surfaceAt = ImVec2(view.origin.x + view.size.x * 0.55f, view.origin.y + view.size.y * 0.2f);
        fprintf(stderr, "medecomp: редактор: демо L44 — картинки кусков %s\n", m_piecesChild ? "ещё рисуются" : "готовы");
        return true;
    }
    bool demoMapShown() const { return m_mapOpen; }
    ImVec2 demoPoint(float dx, float dy) const { return view.toScreen(view.cx + dx, view.cy + dy); }
    ImVec2 demoAddPoint() const { return ImVec2(m_addX, m_addY); }
    ImVec2 demoViewCenter() const { return ImVec2(view.cx, view.cy); }
    void demoPickBag() { m_demoPickBag = true; }
    void demoHand() { setTool(Tool::Hand); }
    void demoMusic() {
        setTool(Tool::Select);
        addCue(roundf(view.cx) - 6, roundf(view.cy) + 2);
        fprintf(stderr, "medecomp: редактор: демо L23 — метка музыки: «%s»\n", soundName(doc.map.musicCues.back().res).c_str());
    }
    float demoZoom() const { return view.zoom; }
    void demoResizeBox() {
        addShape(0, 0, 4, 2);
        view.zoom = 40 * m_scale;
        view.cx = 6, view.cy = 2.5f;
    }
    ImVec2 demoBoxPoint(float fx, float fy) {
        float b[4] = {};
        selBox(doc.map, sel, b);
        return view.toScreen(b[0] + (b[2] - b[0]) * fx, b[1] + (b[3] - b[1]) * fy);
    }
    bool demoGroupArea(const float* a) {
        std::copy(a, a + 4, m_area);
        m_hasArea = true;
        selectInArea();
        return !m_group.empty();
    }
    ImVec2 demoGroupPoint(Kind kind) {
        for (const Sel& s : selection()) {
            if (s.kind != kind) continue;
            const Fix* p = posOf(doc.map, s);
            return kind == Kind::Shape ? view.toScreen((toF(p[0]) + toF(p[2])) / 2, (toF(p[1]) + toF(p[3])) / 2) : view.toScreen(toF(p[0]), toF(p[1]));
        }
        return ImVec2(-1, -1);
    }
    std::string demoGroupText() {
        std::string out = std::to_string(selection().size()) + " (" + groupSummary() + ")";
        for (Kind kind : {Kind::Shape, Kind::Node})
            for (const Sel& s : selection())
                if (s.kind == kind) {
                    const Fix* p = posOf(doc.map, s);
                    char text[80];
                    snprintf(text, sizeof text, "; %s %.2f %.2f", kind == Kind::Shape ? "коробка" : "кусок", toF(p[0]), toF(p[1]));
                    out += text;
                    break;
                }
        return out;
    }
    std::string demoDish(int i) {
        Map& m = doc.map;
        if (i < 0 || size_t(i) >= m.checkpoints.size()) return "нет чекпоинта";
        const Ref& r = m.checkpoints[size_t(i)].ref;
        const Node* d = refNode(m, r);
        if (!d) return "нет тарелки";
        char text[160];
        snprintf(text, sizeof text, "чекпоинт %d (%.2f %.2f): тарелка %d (%.2f %.2f), ссылка %d %d %d %d, ссылок %d, подсветка %d тип %d; кусков %d",
                 i, toF(m.checkpoints[size_t(i)].box[0]), toF(m.checkpoints[size_t(i)].box[1]), d->id, toF(d->pos[0]), toF(d->pos[1]), r.layer,
                 r.column, r.section, r.node, refCount(m, r), d->visionFlag, d->visionType, nodeCount());
        return text;
    }
    void demoOwnDish(int i) { ownDish(i); }
    std::string demoSwing(float dx, float dy) {
        const Start& st = doc.map.starts[0];
        const float x = toF(st.pos[0]) + dx, y = toF(st.pos[1]) + dy;
        setTool(Tool::Swing);
        placeTool(x, y);
        setTool(Tool::Select);
        view.zoom = 40 * m_scale, view.cx = x, view.cy = y - 1;
        char text[96];
        snprintf(text, sizeof text, "%s (%.2f %.2f), предметов %zu", selTitle().c_str(), x, y, doc.map.objects.size());
        return text;
    }
    int demoCheckpointAtStart(float ahead) {
        const Start& st = doc.map.starts[0];
        addCheckpoint(toF(st.pos[0]) - 0.5f + ahead, toF(st.pos[1]));
        return int(doc.map.checkpoints.size()) - 1;
    }
    std::string demoDeleteDish(int i) {
        sel = refSel(doc.map.checkpoints[size_t(i)].ref);
        doc.status.clear();
        deleteSel(&doc, &sel);
        return doc.status.empty() ? "удалён" : doc.status;
    }
    void demoShowCheckpoint(int i) {
        const Fix* b = doc.map.checkpoints[size_t(i)].box;
        select(Kind::Checkpoint, i);
        view.zoom = 40 * m_scale, view.cx = toF(b[0]) + 3, view.cy = toF(b[1]) + 3;
    }
    std::string demoSave() { return save(&doc); }
    ImVec2 demoScreen(float x, float y) { return view.toScreen(x, y); }
    std::string demoSelName() {
        if (!selValid(doc.map, sel)) return "ничего";
        const Fix* p = posOf(doc.map, sel);
        char text[64];
        snprintf(text, sizeof text, " (%.2f %.2f), %d из %d", toF(p[0]), toF(p[1]), m_stackPos + 1, m_stackCount);
        return selTitle() + text;
    }
    std::string demoCounts() { return std::to_string(doc.map.shapes.size()) + " форм, " + std::to_string(nodeCount()) + " кусков"; }
    void demoDeleteGroup() { deleteSelected(); }
    void demoUndo() { undo(); }
    std::string demoBoxText() {
        float b[4];
        if (!selValid(doc.map, sel) || !selBox(doc.map, sel, b)) return "нет рамки";
        char text[96];
        snprintf(text, sizeof text, "%.2f %.2f – %.2f %.2f", b[0], b[1], b[2], b[3]);
        return text;
    }
    bool demoSelectBlock() {
        Map& m = doc.map;
        int best = -1;
        float widest = 0;
        for (size_t i = 0; i < m.shapes.size(); i++) {
            const Shape& s = m.shapes[i];
            const float w = toF(s.box[2]) - toF(s.box[0]);
            if (s.layer == 0 && s.type == 0 && w > widest) best = int(i), widest = w;
        }
        const char* pick = getenv("MEDECOMP_EDITOR_DEMO_L29");
        if (pick && strncmp(pick, "shape:", 6) == 0) {
            best = atoi(pick + 6), widest = 8;
            if (best >= int(m.shapes.size())) best = -1;
        }
        if (const char* info = getenv("MEDECOMP_EDITOR_DEMO_L61")) m_openInfo = std::string("##info-") + info, m_openInfoFrame = ImGui::GetFrameCount() + 3;
        if (best < 0 || view.size.x <= 0) return false;
        select(Kind::Shape, best);
        const Shape& s = m.shapes[size_t(best)];
        view.zoom = view.size.x * 0.5f / widest;
        view.cx = (toF(s.box[0]) + toF(s.box[2])) / 2;
        view.cy = toF(s.box[3]) - view.size.y * 0.2f / view.zoom;
        fprintf(stderr, "medecomp: редактор: демо L29 — коробка %d шириной %.1f; %s\n", best, widest, coverText(s).c_str());
        return true;
    }
    void demoPieceTool() {
        setTool(Tool::Piece);
        int best = -1;
        for (auto& kv : m_paletteIds)
            if (best < 0 || kv.second.first > m_paletteIds[best].first) best = kv.first;
        m_piece = best;
    }
    std::string demoSelected() const {
        if (sel.kind != Kind::Object || !selValid(doc.map, sel)) return "ничего не выбрано";
        const Object& o = doc.map.objects[size_t(sel.index)];
        char text[96];
        snprintf(text, sizeof text, "выбрано: %s в %.2f %.2f", objectName(o.type), toF(o.pos[0]), toF(o.pos[1]));
        return text;
    }
    void demoSelectFirstFilled() {
        Map& m = doc.map;
        for (size_t l = 0; l < m.layers.size(); l++)
            for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++)
                    if (!m.layers[l].columns[c].sections[k].nodes.empty())
                        sel = Sel(), sel.kind = Kind::Node, sel.layer = int(l), sel.column = int(c), sel.section = int(k),
                        sel.index = int(m.layers[l].columns[c].sections[k].nodes.size()) - 1;
    }

private:
    static std::string cacheDir() { return port::files::dataDir() + "/cache"; }
    static std::string previewStamp() { return port::files::gameSource() + "\npieces 9"; }

    void startPieces(const Map& chapter) {
        for (auto& t : m_pieceTex)
            if (t.second.first) SDL_DestroyTexture(t.second.first);
        m_pieceTex.clear();
        for (auto& t : m_frontTex)
            if (t.second.first) SDL_DestroyTexture(t.second.first);
        m_frontTex.clear();
        for (auto& t : m_viewTex)
            if (t.second.first) SDL_DestroyTexture(t.second.first);
        m_viewTex.clear();
        m_pieceFace.clear();
        m_pieceCover.clear();
        m_pieceGroup.clear();
        m_pieceBox.clear();
        m_pieceSides.clear();
        m_flatTop.clear();
        mkdir(cacheDir().c_str(), 0755);
        mkdir((cacheDir() + "/pieces").c_str(), 0755);
        m_piecesDir = cacheDir() + "/pieces/" + doc.palette;
        mkdir(m_piecesDir.c_str(), 0755);
        std::vector<unsigned char> stamp;
        bool fresh = port::files::read(m_piecesDir + "/stamp", &stamp) && std::string(stamp.begin(), stamp.end()) == previewStamp() &&
                     port::files::stat(m_piecesDir + "/done").exists;
#ifdef MEDECOMP_ONE_PROCESS
        m_piecesEnv.clear();
#endif
        if (fresh || m_piecesChild || m_paletteIds.empty()) return loadBoxes();
        std::string ids;
        for (auto& kv : m_paletteIds) ids += (ids.empty() ? "" : ",") + std::to_string(kv.first);
        ids += "," + std::to_string(kBagModel) + "," + std::to_string(kBagModelAlt);
        remove((m_piecesDir + "/done").c_str());
        std::vector<std::string> env = {
            "MEDECOMP_PIECE_PREVIEWS=" + m_piecesDir,
            "MEDECOMP_PIECE_PALETTE=" + std::to_string(chapter.palette) + "," + std::to_string(chapter.modelSet),
            "MEDECOMP_PIECE_IDS=" + ids, "MEDECOMP_PIECE_STAMP=" + previewStamp(), "ALSOFT_DRIVERS=null"};
#ifdef MEDECOMP_ONE_PROCESS
        m_piecesEnv = env;
        return loadBoxes();
#endif
        env.insert(env.end(), {"SDL_VIDEODRIVER=offscreen", "MEDECOMP_WINDOW_SIZE=1024x768"});
        m_piecesChild = port::platform::process::spawn(
            {selfExe(m_argv0), "--no-launcher", "--game", port::files::gameSource()}, env);
    }
    void loadBoxes() {
        std::vector<unsigned char> text;
        if (!port::files::read(m_piecesDir + "/bounds.txt", &text)) return;
        text.push_back(0);
        const char* p = reinterpret_cast<const char*>(text.data());
        int id, n;
        std::array<float, 6> b;
        while (sscanf(p, "%d %f %f %f %f %f %f%n", &id, &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &n) == 7) {
            m_pieceBox[id] = b;
            p += n;
        }
        text.clear();
        if (port::files::read(m_piecesDir + "/sides.txt", &text)) {
            text.push_back(0);
            p = reinterpret_cast<const char*>(text.data());
            std::array<float, 3> s;
            while (sscanf(p, "%d %f %f %f%n", &id, &s[0], &s[1], &s[2], &n) == 4) {
                m_pieceSides[id] = s;
                p += n;
            }
        }
        text.clear();
        if (port::files::read(m_piecesDir + "/surface.txt", &text)) {
            text.push_back(0);
            p = reinterpret_cast<const char*>(text.data());
            float front, left, right;
            int hasFront, hasRoof, hasLeft, hasRight;
            PieceFace f;
            while (sscanf(p, "%d %f %f %f %f %d %d %d %d %f %f %f %f%n", &id, &front, &f.roof, &left, &right, &hasFront, &hasRoof, &hasLeft,
                          &hasRight, &f.lo[0], &f.lo[1], &f.hi[0], &f.hi[1], &n) == 13) {
                f.front = hasFront != 0;
                m_pieceFace[id] = f;
                p += n;
            }
        }
        text.clear();
        if (port::files::read(m_piecesDir + "/views.txt", &text)) {
            text.push_back(0);
            p = reinterpret_cast<const char*>(text.data());
            float cover;
            while (sscanf(p, "%d %f%n", &id, &cover, &n) == 2) {
                m_pieceCover[id] = cover;
                p += n;
            }
        }
        findFlatTops();
        groupPieces();
    }
    void groupPieces() {
        m_pieceGroup.clear();
        std::map<int, std::pair<int, int>> placed;
        for (auto& l : m_paletteMap.layers)
            for (auto& c : l.columns)
                for (auto& sec : c.sections)
                    for (auto& node : sec.nodes) {
                        auto box = m_pieceBox.find(node.id);
                        if (box == m_pieceBox.end()) continue;
                        auto& count = placed[node.id];
                        count.first++;
                        if (toF(node.pos[2]) + box->second[5] < -2.0f) count.second++;
                    }
        for (auto& kv : m_paletteIds) {
            const int id = kv.first;
            auto box = m_pieceBox.find(id);
            int g = kGroupOther;
            if (box != m_pieceBox.end()) {
                const auto& b = box->second;
                const float w = b[3] - b[0], h = b[4] - b[1], d = b[5] - b[2];
                auto where = placed.find(id);
                auto cover = m_pieceCover.find(id);
                auto sides = m_pieceSides.find(id);
                const std::array<float, 3> side = sides == m_pieceSides.end() ? std::array<float, 3>{0, 0, 0} : sides->second;
                if (where != placed.end() && where->second.second * 2 > where->second.first) g = kGroupBack;
                else if (std::max(w, h) <= kPropSize) g = kGroupProps;
                else if (d <= kThinDepth) g = h <= 2.5f ? kGroupRoofs : kGroupWalls;
                else if (h < kLowHeight) g = kGroupRoofs;
                else if (cover != m_pieceCover.end() && cover->second >= 0 && cover->second < kFrameCover) g = kGroupFrames;
                else if (side[2] >= kRoofed) g = kGroupTops;
                else if (std::max(side[0], side[1]) >= kClosedSide) g = kGroupCorners;
                else g = kGroupSections;
            }
            m_pieceGroup[id] = g;
        }
    }
    void findFlatTops() {
        m_flatTop.clear();
        for (auto& l : m_paletteMap.layers)
            for (auto& c : l.columns)
                for (auto& s : c.sections)
                    for (auto& n : s.nodes) {
                        auto b = m_pieceBox.find(n.id);
                        const float z = toF(n.pos[2]);
                        if (b == m_pieceBox.end() || m_flatTop.count(n.id) || z + b->second[2] > 0.5f || z + b->second[5] < -0.5f) continue;
                        const float x0 = toF(n.pos[0]) + b->second[0], x1 = toF(n.pos[0]) + b->second[3];
                        const float top = toF(n.pos[1]) + b->second[4];
                        for (auto& sh : m_paletteMap.shapes) {
                            const float a = toF(sh.box[0]), e = toF(sh.box[2]), d = top - toF(sh.box[3]);
                            if (sh.layer == 0 && sh.type == 0 && std::min(x1, e) - std::max(x0, a) >= 0.85f * (x1 - x0) && d >= -0.05f &&
                                d <= 0.15f) {
                                m_flatTop.insert(n.id);
                                break;
                            }
                        }
                    }
    }
    int capFor(int id, int side) const {
        auto own = m_pieceSides.find(id);
        auto box = m_pieceBox.find(id);
        if (own == m_pieceSides.end() || box == m_pieceBox.end() || own->second[size_t(side)] >= kClosedSide) return -1;
        const auto& b = box->second;
        int best = -1;
        float bestScore = 0;
        for (int c : m_flatTop) {
            auto cs = m_pieceSides.find(c);
            auto cb = m_pieceBox.find(c);
            if (c == id || cs == m_pieceSides.end() || cb == m_pieceBox.end() || cs->second[size_t(side)] < kClosedSide ||
                cs->second[2] < kRoofed)
                continue;
            const auto& k = cb->second;
            if (k[3] - k[0] < 2 || k[4] - k[1] < 4 || k[2] > b[2] + 1.5f || k[5] < b[5] - 1.0f) continue;
            const float dh = fabsf((k[4] - k[1]) - (b[4] - b[1]));
            const float score = (dh < 0.2f && fabsf(k[2] - b[2]) < 0.2f ? 0.0f : 1000.0f) + dh;
            if (best < 0 || score < bestScore) best = c, bestScore = score;
        }
        return best;
    }
    void pollPreviews() {
        if (m_piecesChild && SDL_GetTicks() - m_progressAt > 500) {
            m_progressAt = SDL_GetTicks();
            std::vector<unsigned char> text;
            int done = 0, total = 0;
            if (port::files::read(m_piecesDir + "/progress", &text)) {
                text.push_back(0);
                if (sscanf(reinterpret_cast<const char*>(text.data()), "%d %d", &done, &total) == 2 && total > 0)
                    m_progressDone = done, m_progressTotal = total;
            }
        }
        if (m_piecesChild && port::platform::process::poll(m_piecesChild) != -1) {
            m_progressDone = m_progressTotal = 0;
            m_piecesChild = 0;
            if (FILE* f = fopen((m_piecesDir + "/stamp").c_str(), "wb")) {
                fputs(previewStamp().c_str(), f);
                fclose(f);
            }
            m_pieceTex.clear();
            m_frontTex.clear();
            m_viewTex.clear();
            loadBoxes();
        }
        if (m_mapChild && port::platform::process::poll(m_mapChild) != -1) {
            m_mapChild = 0;
            if (m_mapTex) SDL_DestroyTexture(m_mapTex);
            m_mapTex = loadTexture(cacheDir() + "/map.png", &m_mapSize);
            m_mapOpen = m_mapTex != nullptr;
            if (!m_mapTex) doc.status = L("кадр области не вышел (игра не загрузила уровень?)", "no area shot (did the game load the level?)");
        }
    }
    SDL_Texture* loadTexture(const std::string& path, ImVec2* size) {
        std::vector<unsigned char> bytes;
        port::gfx::Image image;
        if (!port::files::read(path, &bytes) || !port::gfx::decodePNG(bytes.data(), bytes.size(), image, false)) return nullptr;
        SDL_Texture* tex = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, image.width, image.height);
        if (!tex) return nullptr;
        SDL_UpdateTexture(tex, nullptr, image.rgba.data(), image.width * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
        *size = ImVec2(float(image.width), float(image.height));
        return tex;
    }
    bool pieceTexture(int id, SDL_Texture** tex, ImVec2* size) {
        auto it = m_pieceTex.find(id);
        if (it == m_pieceTex.end() && !m_piecesChild) {
            ImVec2 sz;
            SDL_Texture* t = loadTexture(m_piecesDir + "/" + std::to_string(id) + ".png", &sz);
            it = m_pieceTex.insert({id, {t, sz}}).first;
        }
        if (it == m_pieceTex.end() || !it->second.first) return false;
        *tex = it->second.first;
        *size = it->second.second;
        return true;
    }
    bool frontTexture(int id, SDL_Texture** tex, ImVec2* size) {
        auto it = m_frontTex.find(id);
        if (it == m_frontTex.end() && !m_piecesChild) {
            ImVec2 sz;
            SDL_Texture* t = loadTexture(m_piecesDir + "/" + std::to_string(id) + ".front.png", &sz);
            it = m_frontTex.insert({id, {t, sz}}).first;
        }
        if (it == m_frontTex.end() || !it->second.first) return false;
        *tex = it->second.first;
        *size = it->second.second;
        return true;
    }
    bool viewTexture(int id, SDL_Texture** tex) {
        auto it = m_viewTex.find(id);
        if (it == m_viewTex.end() && !m_piecesChild) {
            ImVec2 sz;
            SDL_Texture* t = loadTexture(m_piecesDir + "/" + std::to_string(id) + ".view.png", &sz);
            it = m_viewTex.insert({id, {t, sz}}).first;
        }
        if (it == m_viewTex.end() || !it->second.first) return false;
        *tex = it->second.first;
        return true;
    }
    bool nodeBox(const Node& n, std::array<float, 6>* out) const {
        auto box = m_pieceBox.find(n.id);
        if (box == m_pieceBox.end()) return false;
        *out = box->second;
        if (n.mirror) (*out)[0] = -box->second[3], (*out)[3] = -box->second[0];
        return true;
    }
    void drawPieceViews(ImDrawList* dl) {
        struct Item {
            float z;
            int id;
            bool mirror;
            ImVec2 a, b;
        };
        std::vector<Item> items;
        for (auto& l : doc.map.layers)
            for (auto& c : l.columns)
                for (auto& sec : c.sections)
                    for (auto& node : sec.nodes) {
                        std::array<float, 6> box;
                        if (!nodeBox(node, &box)) continue;
                        const float x = toF(node.pos[0]), y = toF(node.pos[1]);
                        const ImVec2 a = view.toScreen(x + box[0], y + box[4]), b = view.toScreen(x + box[3], y + box[1]);
                        if (b.x < view.origin.x || a.x > view.origin.x + view.size.x || b.y < view.origin.y || a.y > view.origin.y + view.size.y)
                            continue;
                        items.push_back({toF(node.pos[2]) + (box[2] + box[5]) / 2, node.id, node.mirror, a, b});
                    }
        std::stable_sort(items.begin(), items.end(), [](const Item& p, const Item& q) { return p.z < q.z; });
        for (const Item& it : items) {
            SDL_Texture* tex = nullptr;
            if (viewTexture(it.id, &tex))
                dl->AddImage(ImTextureRef(ImTextureID(tex)), it.a, it.b, ImVec2(it.mirror ? 1.0f : 0.0f, 0), ImVec2(it.mirror ? 0.0f : 1.0f, 1));
        }
    }
    bool drawSurface(ImDrawList* dl, const Shape& s) {
        const int id = port::platform::shapeSurface(s.material);
        auto face = m_pieceFace.find(id);
        SDL_Texture* tex = nullptr;
        ImVec2 size;
        if (!id || face == m_pieceFace.end() || !face->second.front || !frontTexture(id, &tex, &size)) return false;
        const PieceFace& f = face->second;
        const float r = f.roof, imgW = std::max(0.01f, f.hi[0] - f.lo[0]), imgH = std::max(0.01f, f.hi[1] - f.lo[1]);
        const float w = std::max(kSurfaceStep, imgW);
        float h = r - f.lo[1];
        if (h < kSurfaceStep) h = std::max(kSurfaceStep, imgH);
        const float x0 = toF(s.box[0]), y0 = toF(s.box[1]), x1 = toF(s.box[2]), y1 = toF(s.box[3]);
        if (x1 <= x0 || y1 <= y0) return false;
        const int nx = std::max(1, int(ceilf((x1 - x0) / w - 1e-4f))), ny = std::max(1, int(ceilf((y1 - y0) / h - 1e-4f)));
        if (nx * ny > kSurfaceTiles) return false;
        struct Half {
            float a, b, d;
        };
        std::vector<Half> ramp;
        if (s.type == 1 || s.type == 2) {
            const float hx = s.type == 1 ? x0 : x1, k = (y0 - y1) / ((s.type == 1 ? x1 : x0) - hx);
            ramp.push_back({-k, 1, y1 - k * hx});
        }
        dl->PushTexture(ImTextureRef(ImTextureID(tex)));
        std::vector<ImVec2> poly, next;
        for (int i = 0; i < nx; i++)
            for (int j = 0; j < ny; j++) {
                const float X = x0 + float(i) * w, Y = y1 - float(j) * h, top = Y + (f.hi[1] - r);
                std::vector<Half> planes = ramp;
                planes.push_back({-1, 0, -X});
                planes.push_back({1, 0, std::min({x1, X + w, X + imgW})});
                planes.push_back({0, -1, -std::max({y0, Y - h, top - imgH})});
                planes.push_back({0, 1, j == 0 && s.type == 0 ? std::min(top, y1 + port::platform::kAboveTop) : Y});
                poly = {ImVec2(X, top - imgH), ImVec2(X + imgW, top - imgH), ImVec2(X + imgW, top), ImVec2(X, top)};
                for (const Half& p : planes) {
                    next.clear();
                    for (size_t k = 0; k < poly.size(); k++) {
                        const ImVec2 a = poly[k], c = poly[(k + 1) % poly.size()];
                        const float da = p.a * a.x + p.b * a.y - p.d, dc = p.a * c.x + p.b * c.y - p.d;
                        if (da <= 0) next.push_back(a);
                        if ((da < 0) != (dc < 0) && da != dc) {
                            const float t = da / (da - dc);
                            next.push_back(ImVec2(a.x + (c.x - a.x) * t, a.y + (c.y - a.y) * t));
                        }
                    }
                    poly.swap(next);
                    if (poly.size() < 3) break;
                }
                if (poly.size() < 3) continue;
                const int n = int(poly.size());
                dl->PrimReserve((n - 2) * 3, n);
                const ImDrawIdx first = ImDrawIdx(dl->_VtxCurrentIdx);
                for (const ImVec2& q : poly)
                    dl->PrimWriteVtx(view.toScreen(q.x, q.y),
                                     ImVec2(std::min(1.0f, std::max(0.0f, (q.x - X) / imgW)), std::min(1.0f, std::max(0.0f, (top - q.y) / imgH))), IM_COL32_WHITE);
                for (int k = 1; k + 1 < n; k++) {
                    dl->PrimWriteIdx(first);
                    dl->PrimWriteIdx(ImDrawIdx(first + k));
                    dl->PrimWriteIdx(ImDrawIdx(first + k + 1));
                }
            }
        dl->PopTexture();
        return true;
    }
    void pieceImage(int id, float maxW = 0, float maxH = 0) {
        SDL_Texture* tex = nullptr;
        ImVec2 size;
        if (!pieceTexture(id, &tex, &size)) {
            ImGui::TextDisabled("%s", m_piecesChild ? L("превью готовятся…", "previews are being made…")
                                                    : L("превью нет", "no preview"));
            return;
        }
        float k = std::min((maxW > 0 ? maxW : ImGui::GetContentRegionAvail().x) / size.x, (maxH > 0 ? maxH : 160 * m_scale) / size.y);
        ImGui::Image(ImTextureID(tex), ImVec2(size.x * k, size.y * k));
        auto box = m_pieceBox.find(id);
        if (box != m_pieceBox.end())
            ImGui::TextDisabled("%.1f × %.1f × %.1f", box->second[3] - box->second[0], box->second[4] - box->second[1],
                                box->second[5] - box->second[2]);
    }
    void mapWindow() {
        if (!m_mapOpen || !m_mapTex) return;
        ImGui::SetNextWindowPos(ImVec2(360 * m_scale, 30 * m_scale), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(660 * m_scale, 540 * m_scale), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(L("Как в игре", "As in the game"), &m_mapOpen)) {
            float k = ImGui::GetContentRegionAvail().x / m_mapSize.x;
            ImGui::Image(ImTextureID(m_mapTex), ImVec2(m_mapSize.x * k, m_mapSize.y * k));
        }
        ImGui::End();
    }

    void area(float* a) const {
        a[0] = std::min(m_area[0], m_area[2]), a[2] = std::max(m_area[0], m_area[2]);
        a[1] = std::min(m_area[1], m_area[3]), a[3] = std::max(m_area[1], m_area[3]);
    }
    void fillArea() {
        Map& m = doc.map;
        if (!selValid(m, sel) || (sel.kind != Kind::Shape && sel.kind != Kind::Node)) return;
        float a[4], w = 4, h = 4, ox = 0, oy = 0;
        area(a);
        Shape shape;
        Node node;
        if (sel.kind == Kind::Shape) {
            shape = m.shapes[size_t(sel.index)];
            w = toF(shape.box[2]) - toF(shape.box[0]), h = toF(shape.box[3]) - toF(shape.box[1]);
        } else {
            node = m.layers[size_t(sel.layer)].columns[size_t(sel.column)].sections[size_t(sel.section)].nodes[size_t(sel.index)];
            node.visionFlag = 0, node.visionType = 0;
            node.shared = 0;
            std::fill(node.visionBox, node.visionBox + 4, 0);
            std::array<float, 6> box;
            if (nodeBox(node, &box)) w = box[3] - box[0], h = box[4] - box[1], ox = box[0], oy = box[1];
        }
        if (w < kSnap || h < kSnap) return void(doc.status = L("у тайла нет размера", "the tile has no size"));
        int nx = std::max(1, int((a[2] - a[0]) / w + 0.01f)), ny = std::max(1, int((a[3] - a[1]) / h + 0.01f));
        if (nx * ny > 400) return void(doc.status = L("больше 400 тайлов — возьмите область меньше", "more than 400 tiles — take a smaller area"));
        pushUndo(&doc);
        for (int j = 0; j < ny; j++)
            for (int i = 0; i < nx; i++) {
                float x = a[0] + i * w, y = a[1] + j * h;
                if (sel.kind == Kind::Shape) {
                    Shape s = shape;
                    s.box[0] = toFix(x), s.box[1] = toFix(y), s.box[2] = toFix(x + w), s.box[3] = toFix(y + h);
                    m.shapes.push_back(s);
                } else {
                    Node n = node;
                    n.pos[0] = toFix(x - ox), n.pos[1] = toFix(y - oy);
                    Sel at;
                    sectionAt(m, x - ox, &at);
                    m.layers[size_t(at.layer)].columns[size_t(at.column)].sections[size_t(at.section)].nodes.push_back(n);
                }
            }
        doc.status = std::string(L("поставлено: ", "placed: ")) + std::to_string(nx * ny);
    }
    void clearArea() {
        Map& m = doc.map;
        float a[4];
        area(a);
        auto in = [&](Fix x, Fix y) { return toF(x) >= a[0] && toF(x) <= a[2] && toF(y) >= a[1] && toF(y) <= a[3]; };
        pushUndo(&doc);
        size_t removed = m.shapes.size(), kept = 0;
        m.shapes.erase(std::remove_if(m.shapes.begin(), m.shapes.end(),
                                      [&](const Shape& s) { return in(s.box[0], s.box[1]) && in(s.box[2], s.box[3]); }),
                       m.shapes.end());
        removed -= m.shapes.size();
        for (size_t l = 0; l < m.layers.size(); l++)
            for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++) {
                    auto& nodes = m.layers[l].columns[c].sections[k].nodes;
                    for (size_t n = nodes.size(); n-- > 0;) {
                        if (!in(nodes[n].pos[0], nodes[n].pos[1])) continue;
                        if (nodes[n].visionFlag == 1 || nodes.size() <= 1) kept++;
                        else eraseNode(m, int(l), int(c), int(k), int(n)), removed++;
                    }
                }
        if (!removed) doc.undo.pop_back();
        sel = Sel();
        doc.status = std::string(L("удалено: ", "removed: ")) + std::to_string(removed) +
                     (kept ? std::string(L(", осталось с подсветкой или последних в секции: ", ", kept (highlight or last in section): ")) + std::to_string(kept) : "");
    }
    void previewArea() {
        if (m_mapChild) return;
        doSave();
        float a[4];
        area(a);
        char region[128];
        snprintf(region, sizeof region, "%.2f,%.2f,%.2f,%.2f", a[0], a[1], a[2], a[3]);
        mkdir(cacheDir().c_str(), 0755);
        remove((cacheDir() + "/map.png").c_str());
#ifdef MEDECOMP_ONE_PROCESS
        requestService({"MEDECOMP_MAP_PREVIEW=" + cacheDir() + "/map.png", std::string("MEDECOMP_MAP_REGION=") + region,
                        "ALSOFT_DRIVERS=null"},
                       doc.id, "map");
        return;
#endif
        m_mapChild = port::platform::process::spawn(
            {selfExe(m_argv0), "--game", port::files::gameSource(), "--level", doc.id},
            {"MEDECOMP_MAP_PREVIEW=" + cacheDir() + "/map.png", std::string("MEDECOMP_MAP_REGION=") + region,
             "SDL_VIDEODRIVER=offscreen", "ALSOFT_DRIVERS=null", "MEDECOMP_WINDOW_SIZE=1024x768"});
        if (!m_mapChild) doc.status = L("игра не запустилась", "the game did not start");
    }

    void editorView() {
        rail();
        ImGui::SameLine(0, 0);
        ImGui::BeginChild("##view", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        canvas();
        addHereMenu();
        ImGui::EndChild();
        shortcuts();
    }

    void undo() {
        if (doc.undo.empty()) return;
        doc.map = doc.undo.back();
        doc.undo.pop_back();
        if (!selValid(doc.map, sel)) sel = Sel();
    }

    void shortcuts() {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput) return;
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) undo();
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) doSave();
        if (ImGui::IsKeyPressed(ImGuiKey_F5)) test();
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) deleteSelected();
        if (io.KeyCtrl || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) return;
        if (ImGui::IsKeyPressed(ImGuiKey_V) || ImGui::IsKeyPressed(ImGuiKey_Escape)) setTool(Tool::Select);
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_group.clear();
        if (ImGui::IsKeyPressed(ImGuiKey_H)) setTool(Tool::Hand);
        if (ImGui::IsKeyPressed(ImGuiKey_A)) setTool(Tool::Area);
        if (ImGui::IsKeyPressed(ImGuiKey_B)) setTool(Tool::Block);
        if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) zoomBy(1.25f);
        if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) zoomBy(1 / 1.25f);
        if (ImGui::IsKeyPressed(ImGuiKey_0) || ImGui::IsKeyPressed(ImGuiKey_Keypad0)) fit();
    }

    void setTool(Tool tool) {
        m_tool = tool;
        m_areaDrag = false;
    }
    void deleteSelected() {
        if (m_group.empty()) {
            deleteSel(&doc, &sel);
            return;
        }
        deleteGroup(&doc, selection());
        sel = Sel(), m_group.clear(), m_groupFor = Sel();
    }

    bool supportsShown() const { return doc.supports == "visible" || (doc.supports.empty() && doc.origin != "chapter"); }

    void closeLevel() {
        doc = Doc();
        sel = Sel();
        m_tool = Tool::Select;
        m_hasArea = m_areaDrag = false;
    }

    void placeTool(float x, float y) {
        switch (m_tool) {
        case Tool::Block: addShape(0, 0, x, y); break;
        case Tool::RampL: addShape(1, 0, x, y); break;
        case Tool::RampR: addShape(2, 0, x, y); break;
        case Tool::Bag: addObject(kBag, x, y); break;
        case Tool::Checkpoint: addCheckpoint(x, y); break;
        case Tool::Zip: addZip(x, y); break;
        case Tool::Swing: addObject(kSwing, x, y); break;
        case Tool::Music: addCue(x, y); break;
        case Tool::Piece:
            if (m_paletteIds.count(m_piece)) addPiece(m_piece, x, y);
            else doc.status = L("выберите кусок в палитре слева", "pick a piece in the palette on the left");
            break;
        default: break;
        }
    }

    const char* toolName(Tool tool) const {
        switch (tool) {
        case Tool::Block: return L("Коробка", "Block");
        case Tool::RampL: return L("Рампа к −x", "Ramp to −x");
        case Tool::RampR: return L("Рампа к +x", "Ramp to +x");
        case Tool::Bag: return L("Сумка", "Bag");
        case Tool::Checkpoint: return L("Чекпоинт", "Checkpoint");
        case Tool::Zip: return L("Трос", "Zip line");
        case Tool::Swing: return L("Турник", "Swing bar");
        case Tool::Music: return L("Музыкальная метка", "Music cue");
        case Tool::Piece: return L("Кусок палитры", "Palette piece");
        case Tool::Area: return L("Область", "Area");
        case Tool::Hand: return L("Рука — двигать вид", "Hand — pan the view");
        default: return L("Выбрать и тащить", "Select and drag");
        }
    }

    void ghost(ImDrawList* dl, float x, float y) {
        const ImU32 line = tok(mcui::tokens::kColorAccent), fill = tok(mcui::tokens::kColorAccent, 0.18f);
        auto rect = [&](float ax, float ay, float bx, float by) {
            dl->AddRectFilled(view.toScreen(ax, by), view.toScreen(bx, ay), fill);
            dl->AddRect(view.toScreen(ax, by), view.toScreen(bx, ay), line, 0.0f, ImDrawFlags_None, 1.5f * m_scale);
        };
        auto tri = [&](ImVec2 a, ImVec2 b, ImVec2 c) {
            dl->AddTriangleFilled(a, b, c, fill);
            dl->AddTriangle(a, b, c, line, 1.5f * m_scale);
        };
        const float r = 6 * m_scale;
        switch (m_tool) {
        case Tool::Block: rect(x, y, x + 4, y + 1); break;
        case Tool::RampL: tri(view.toScreen(x, y), view.toScreen(x + 4, y), view.toScreen(x, y + 2)); break;
        case Tool::RampR: tri(view.toScreen(x, y), view.toScreen(x + 4, y), view.toScreen(x + 4, y + 2)); break;
        case Tool::Checkpoint: {
            const Node* dish = nullptr;
            if (const Checkpoint* first = checkpointModel(&dish)) {
                const Fix* b = first->box;
                rect(x, y, x + toF(b[2]) - toF(b[0]), y + toF(b[3]) - toF(b[1]));
            }
            break;
        }
        case Tool::Zip:
            dl->AddLine(view.toScreen(x, y + 4), view.toScreen(x + 16, y), line, 2 * m_scale);
            dl->AddCircleFilled(view.toScreen(x, y + 4), r, fill);
            break;
        case Tool::Swing:
            dl->AddLine(view.toScreen(x - kSwingHalf, y), view.toScreen(x + kSwingHalf, y), line, 3 * m_scale);
            dl->AddLine(view.toScreen(x, y), view.toScreen(x, y - kSwingHang), fill, 1.5f * m_scale);
            break;
        case Tool::Piece: {
            auto b = m_pieceBox.find(m_piece);
            if (b != m_pieceBox.end()) rect(x + b->second[0], y + b->second[1], x + b->second[3], y + b->second[4]);
            else rect(x - 0.5f, y - 0.5f, x + 0.5f, y + 0.5f);
            break;
        }
        default:
            dl->AddCircleFilled(view.toScreen(x, y), r, fill);
            dl->AddCircle(view.toScreen(x, y), r, line, 0, 1.5f * m_scale);
            break;
        }
        std::string tag = std::string(toolName(m_tool)) + L(" · щелчок — поставить, Esc — выбор", " · click to place, Esc — select");
        ImVec2 at(ImGui::GetIO().MousePos.x + 16 * m_scale, ImGui::GetIO().MousePos.y + 18 * m_scale);
        ImVec2 size = ImGui::CalcTextSize(tag.c_str());
        const float pad = 6 * m_scale;
        dl->AddRectFilled(ImVec2(at.x - pad, at.y - pad / 2), ImVec2(at.x + size.x + pad, at.y + size.y + pad / 2),
                          tok(mcui::tokens::kColorBgPrimary, 0.92f), 8 * m_scale);
        dl->AddText(at, kAccentLight, tag.c_str());
    }

    bool iconButton(const char* id, const mcui::Icon& icon, float size, ImU32 color, ImU32 fill = 0, const char* tip = nullptr) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        bool pressed = ImGui::InvisibleButton(id, ImVec2(size, size));
        bool hovered = ImGui::IsItemHovered();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 bg = fill ? fill : hovered ? tok(mcui::tokens::kColorBgHover) : 0;
        if (bg) dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size), faded(bg), std::min(12 * m_scale, size / 3));
        float is = std::min(20 * m_scale, size * 0.62f);
        mcui::DrawIcon(dl, icon, ImVec2(p.x + (size - is) / 2, p.y + (size - is) / 2), is, faded(color));
        if (tip && !m_touchSeen && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", tip);
        return pressed;
    }

    void infoButton(const char* id, const char* text) {
        const float s = m_scale, size = ImGui::GetFrameHeight();
        ImGui::SameLine(0, 4 * s);
        const bool pressed = iconButton(id, icons().info, size, tok(mcui::tokens::kColorTextMuted), 0, nullptr);
        if (pressed || (m_openInfo == id && ImGui::GetFrameCount() >= m_openInfoFrame)) {
            m_openInfo.clear();
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMax().x, ImGui::GetItemRectMin().y), ImGuiCond_Always, ImVec2(1, 1));
            ImGui::OpenPopup(id);
        }
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(300 * s, FLT_MAX));
        if (ImGui::BeginPopup(id)) {
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 280 * s);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndPopup();
        }
    }

    void rail() {
        const float s = m_scale, w = 64 * s;
        const Icons& ic = icons();
        const ImU32 text = tok(mcui::tokens::kColorTextSecondary);
        struct Item {
            Tool tool;
            const mcui::Icon* icon;
            ImU32 color;
            const char* key;
        };
        const Item items[] = {{Tool::Select, &ic.select, text, " · V"},
                              {Tool::Hand, &ic.hand, text, L(" · H или пробел, колесо, два пальца", " · H or Space, wheel, two fingers")},
                              {Tool::Area, &ic.area, text, L(" · A или Shift+ЛКМ", " · A or Shift+LMB")},
                              {Tool::Block, &ic.block, text, L(" · B — по ней бегают и прыгают", " · B — you run and jump on it")},
                              {Tool::RampL, &ic.rampL, text, L(" — по ней бегают и прыгают", " — you run and jump on it")},
                              {Tool::RampR, &ic.rampR, text, L(" — по ней бегают и прыгают", " — you run and jump on it")},
                              {Tool::Bag, &ic.bag, tok(mcui::tokens::kColorAttention), L(" — в режиме «Сбор сумок»", " — in the Bag collection mode")},
                              {Tool::Checkpoint, &ic.flag, kAccentLight, ""}, {Tool::Zip, &ic.zip, text, ""},
                              {Tool::Swing, &ic.swing, text, L(" — повиснуть и раскачаться (сама точка не видна)", " — hang and swing (the point itself is invisible)")},
                              {Tool::Music, &ic.music, text, L(" — невидимая", " — invisible")},
                              {Tool::Piece, &ic.piece, text, L(" — видимая модель из главы уровня", " — a visible model from the level's chapter")}};
        const size_t count = sizeof items / sizeof items[0];
        const float gap = (compactMobile() ? 2 : 6) * s;
        const float fitButton = (ImGui::GetContentRegionAvail().y - 28 * s - (count + 1) * gap) / (count + 1);
        const float b = compactMobile() ? std::min(44 * s, std::max(20.0f, fitButton)) : 44 * s;
        const bool scrollRail = compactMobile() && fitButton < 20;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, mcui::Color(mcui::tokens::kColorBgSecondary));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2((w - b) / 2, 12 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, gap));
        ImGui::BeginChild("##rail", ImVec2(w, 0), ImGuiChildFlags_AlwaysUseWindowPadding,
                          scrollRail ? 0 : ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (scrollRail) port::platform::panelTouchScroll(s);
        for (size_t i = 0; i < count; i++) {
            if (i == 3) {
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + 8 * s, p.y + 2 * s), ImVec2(p.x + b - 8 * s, p.y + 2 * s),
                                                    tok(mcui::tokens::kColorBgHover), 1.0f);
                ImGui::Dummy(ImVec2(b, 4 * s));
            }
            const Item& it = items[i];
            const bool on = m_tool == it.tool;
            std::string tip = std::string(toolName(it.tool)) + (m_touchSeen ? "" : it.key);
            ImGui::PushID(int(i));
            if (iconButton("##tool", *it.icon, b, on ? IM_COL32_WHITE : it.color, on ? tok(mcui::tokens::kColorAccent) : 0, tip.c_str()))
                setTool(it.tool);
            ImGui::PopID();
        }
        ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - b - 12 * s));
        if (iconButton("##close", ic.close, b, tok(mcui::tokens::kColorTextMuted), 0,
                       L("Закрыть уровень — несохранённое пропадёт", "Close the level — unsaved changes are lost")))
            closeLevel();
        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }

    void overlays() {
        depthStrip();
        levelCard();
        actionsBar();
        if (m_tool == Tool::Piece) piecesFlyout();
        areaBar();
        inspector();
        surfaceChip();
        surfacePopup();
        viewBar();
    }

    enum class Depth { Unknown, Back, Run, Front };
    Depth depthOf(const Node& n) const {
        auto box = m_pieceBox.find(n.id);
        if (box == m_pieceBox.end()) return Depth::Unknown;
        const float z = toF(n.pos[2]), back = z + box->second[2], front = z + box->second[5];
        return front < 0 ? Depth::Back : back > 0 ? Depth::Front : Depth::Run;
    }
    ImU32 depthColor(Depth d, float alpha) const {
        switch (d) {
        case Depth::Front: return tok(mcui::tokens::kColorAttention, alpha);
        case Depth::Back: return IM_COL32(150, 160, 180, int(255 * alpha));
        default: return tok(mcui::tokens::kColorAccent, alpha);
        }
    }
    void depthRow(Node& n) {
        auto box = m_pieceBox.find(n.id);
        const Depth now = depthOf(n);
        fieldLabel(L("глубина", "depth"));
        const float gap = ImGui::GetStyle().ItemSpacing.x, third = (ImGui::GetContentRegionAvail().x - 2 * gap) / 3;
        ImGui::BeginDisabled(box == m_pieceBox.end());
        struct Choice {
            Depth d;
            const char* label;
        };
        const Choice choices[] = {{Depth::Back, L("Фон", "Back")}, {Depth::Run, L("Линия бега", "Run line")}, {Depth::Front, L("Перед", "Front")}};
        for (int i = 0; i < 3; i++) {
            if (i) ImGui::SameLine(0, gap);
            const Choice& c = choices[i];
            if (mcui::Button(c.label, now == c.d ? mcui::ButtonKind::Accent : mcui::ButtonKind::Default, ImVec2(third, 0)) &&
                box != m_pieceBox.end()) {
                pushUndo(&doc);
                const float* b = box->second.data();
                n.pos[2] = toFix(c.d == Depth::Back ? kBackFront - b[5] : c.d == Depth::Front ? kFrontBack - b[2] : kRunFront - b[5]);
            }
        }
        ImGui::EndDisabled();
        if (box == m_pieceBox.end()) {
            caption(L("размера куска ещё нет — картинки кусков готовятся", "the piece has no size yet — previews are being made"));
            return;
        }
        caption(now == Depth::Back    ? L("За линией бега: фон, героиню не закрывает, по нему не бегают.",
                                          "Behind the run line: background, never covers the heroine, nothing to run on.")
                : now == Depth::Front ? L("Перед линией бега: ближе героини, может её закрыть.",
                                          "In front of the run line: nearer than the heroine, may cover her.")
                                      : L("На линии бега: там же, где коробки и героиня, — по его крыше и полу бегут.",
                                          "On the run line: where the blocks and the heroine are — its roof and floor are run on."));
    }
    void mirrorRow(Node& n) {
        std::array<float, 6> box;
        const bool sized = nodeBox(n, &box);
        ImGui::BeginDisabled(!sized);
        if (mcui::Button(n.mirror ? L("Отражён — вернуть", "Mirrored — undo") : L("Отразить слева направо", "Mirror left to right"),
                         n.mirror ? mcui::ButtonKind::Accent : mcui::ButtonKind::Default, ImVec2(-FLT_MIN, 0)) && sized) {
            pushUndo(&doc);
            n.pos[0] = toFix(toF(n.pos[0]) + box[0] + box[3]);
            n.mirror = !n.mirror;
        }
        ImGui::EndDisabled();
        caption(L("Только вид: коробки и рампы, по которым бегут, не меняются. Версии игры до этой такой кусок не покажут.",
                  "Looks only: the blocks and ramps you run on stay as they are. Game versions before this one do not show such a piece."));
    }
    bool depthShown() const { return showDepth && showGraphics && doc.open && view.size.x >= 232 * m_scale && view.size.y >= 360 * m_scale; }
    float depthHeight() const { return compactMobile() ? std::min(120 * m_scale, view.size.y * .18f) : 120 * m_scale; }
    float viewBarReserve() const { return std::max(56 * m_scale, m_viewBarH + 12 * m_scale); }
    float depthReserve() const {
        return depthShown() ? viewBarReserve() + depthHeight() + 12 * m_scale : compactMobile() ? viewBarReserve() : 0.0f;
    }
    void depthStrip() {
        if (!depthShown()) return;
        Map& m = doc.map;
        const float sc = m_scale, edge = 16 * sc, h = depthHeight();
        const float left = view.origin.x + edge + (m_tool == Tool::Piece ? (kPaletteWidth + 12) * sc : 0);
        const ImVec2 at(left, view.origin.y + view.size.y - edge - viewBarReserve());
        const float w = view.origin.x + view.size.x - edge - left;
        if (w < 200 * sc) return;
        ImGui::SetNextWindowPos(at, ImGuiCond_Always, ImVec2(0, 1));
        ImGui::SetNextWindowSize(ImVec2(w, h));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12 * sc);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, mcui::Color(mcui::tokens::kColorBgSecondary, 0.92f));
        ImGui::PushStyleColor(ImGuiCol_Border, mcui::Color(mcui::tokens::kColorBgTertiary));
        ImGui::Begin("##depth", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        const ImVec2 o = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##depthfield", ImVec2(w, h));
        const bool hovered = ImGui::IsItemHovered();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float top = o.y + 6 * sc, bottom = o.y + h - 6 * sc, ppu = (bottom - top) / (kDepthNear - kDepthFar);
        auto yOf = [&](float z) { return top + (std::min(kDepthNear, std::max(kDepthFar, z)) - kDepthFar) * ppu; };
        dl->PushClipRect(o, ImVec2(o.x + w, o.y + h), true);
        dl->AddRectFilled(ImVec2(o.x, yOf(kRunBack)), ImVec2(o.x + w, yOf(kRunFront)), tok(mcui::tokens::kColorAccent, 0.07f));
        dl->AddLine(ImVec2(o.x, yOf(0)), ImVec2(o.x + w, yOf(0)), tok(mcui::tokens::kColorAccent, 0.55f));
        if (showShapes)
            for (const Shape& s : m.shapes) {
                if (s.layer != 0) continue;
                const float x0 = view.toScreen(toF(s.box[0]), 0).x, x1 = view.toScreen(toF(s.box[2]), 0).x;
                if (x1 < o.x || x0 > o.x + w) continue;
                dl->AddRectFilled(ImVec2(x0, yOf(kRunBack)), ImVec2(x1, yOf(kRunFront)), IM_COL32(200, 205, 215, 26));
            }
        struct Item {
            float zc;
            Depth d;
            ImVec2 a, b;
            Sel at;
        };
        std::vector<Item> items;
        for (size_t l = 0; l < m.layers.size(); l++)
            for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++) {
                    auto& nodes = m.layers[l].columns[c].sections[k].nodes;
                    for (size_t i = 0; i < nodes.size(); i++) {
                        std::array<float, 6> box;
                        if (!nodeBox(nodes[i], &box)) continue;
                        const float x = toF(nodes[i].pos[0]), z = toF(nodes[i].pos[2]);
                        const float x0 = view.toScreen(x + box[0], 0).x, x1 = view.toScreen(x + box[3], 0).x;
                        if (x1 < o.x || x0 > o.x + w) continue;
                        Sel s;
                        s.kind = Kind::Node, s.index = int(i), s.layer = int(l), s.column = int(c), s.section = int(k);
                        items.push_back({z + (box[2] + box[5]) / 2, depthOf(nodes[i]), ImVec2(x0, yOf(z + box[2])), ImVec2(x1, yOf(z + box[5])), s});
                    }
                }
        std::stable_sort(items.begin(), items.end(), [](const Item& p, const Item& q) { return p.zc < q.zc; });
        auto same = [](const Sel& a, const Sel& b) {
            return a.kind == b.kind && a.index == b.index && a.layer == b.layer && a.column == b.column && a.section == b.section;
        };
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        int under = -1;
        m_depthSelAt = ImVec2(-1, -1);
        for (size_t i = 0; i < items.size(); i++) {
            const Item& it = items[i];
            const bool on = same(sel, it.at);
            if (on) m_depthSelAt = ImVec2((it.a.x + it.b.x) / 2, (it.a.y + it.b.y) / 2);
            dl->AddRectFilled(it.a, it.b, depthColor(it.d, on ? 0.45f : 0.22f));
            dl->AddRect(it.a, it.b, on ? IM_COL32_WHITE : depthColor(it.d, 0.7f), 0.0f, ImDrawFlags_None, on ? 2.0f : 1.0f);
            if (hovered && mouse.x >= it.a.x && mouse.x <= it.b.x && mouse.y >= it.a.y - 2 && mouse.y <= it.b.y + 2) under = int(i);
        }
        ImGui::PushFont(nullptr, mcui::kTextCaption);
        const ImU32 muted = tok(mcui::tokens::kColorTextMuted);
        auto label = [&](const char* text, float y) {
            const ImVec2 ts = ImGui::CalcTextSize(text);
            dl->AddText(ImVec2(o.x + w - ts.x - 10 * sc, y - ts.y / 2), muted, text);
        };
        label(L("фон", "back"), (top + yOf(kRunBack)) / 2);
        label(L("линия бега", "run line"), yOf((kRunBack + kRunFront) / 2));
        label(L("перед", "front"), (yOf(kRunFront) + bottom) / 2);
        dl->AddText(ImVec2(o.x + 10 * sc, top), muted, L("вид сверху · тяните кусок, чтобы менять глубину", "top view · drag a piece to change its depth"));
        ImGui::PopFont();
        dl->PopClipRect();
        if (under >= 0 && !ImGui::IsItemActive()) {
            const Item& it = items[size_t(under)];
            const Node& n = m.layers[size_t(it.at.layer)].columns[size_t(it.at.column)].sections[size_t(it.at.section)].nodes[size_t(it.at.index)];
            ImGui::SetTooltip("%s %d · z %.2f", L("кусок", "piece"), n.id, toF(n.pos[2]));
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            m_depthDrag = false;
            if (under >= 0) {
                sel = items[size_t(under)].at;
                m_depthDrag = true;
                m_depthStartY = mouse.y;
                m_depthStartZ = toF(m.layers[size_t(sel.layer)].columns[size_t(sel.column)].sections[size_t(sel.section)].nodes[size_t(sel.index)].pos[2]);
                m_depthUndo = false;
            }
        }
        if (m_depthDrag && ImGui::IsItemActive() && sel.kind == Kind::Node && selValid(m, sel)) {
            Node& n = m.layers[size_t(sel.layer)].columns[size_t(sel.column)].sections[size_t(sel.section)].nodes[size_t(sel.index)];
            const float z = roundf((m_depthStartZ + (mouse.y - m_depthStartY) / ppu) / kSnap) * kSnap;
            if (fabsf(z - toF(n.pos[2])) > 1e-4f) {
                if (!m_depthUndo) pushUndo(&doc), m_depthUndo = true;
                n.pos[2] = toFix(z);
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
        if (!ImGui::IsItemActive()) m_depthDrag = false;
        ImGui::End();
    }

    void surfaceRow(Shape& s) {
        const float sc = m_scale;
        const int id = port::platform::shapeSurface(s.material);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(L("поверхность", "surface"));
        ImGui::SameLine();
        SDL_Texture* tex = nullptr;
        ImVec2 size;
        auto face = m_pieceFace.find(id);
        if (id && face != m_pieceFace.end() && frontTexture(id, &tex, &size)) {
            const float bw = face->second.hi[0] - face->second.lo[0], bh = std::max(0.01f, face->second.hi[1] - face->second.lo[1]);
            ImGui::Image(ImTextureRef(ImTextureID(tex)), ImVec2(std::min(28 * sc * bw / bh, 64 * sc), 28 * sc));
            ImGui::SameLine();
        }
        ImGui::AlignTextToFramePadding();
        if (id) ImGui::Text("%s %d", L("кусок", "piece"), id);
        else ImGui::TextDisabled("%s", L("нет", "none"));
        ImGui::BeginDisabled(m_paletteIds.empty());
        if (!id) {
            ImGui::SameLine();
            if (mcui::Button(L("Выбрать…", "Choose…"))) m_openSurface = true, m_surfaceAt = ImGui::GetItemRectMin();
        } else {
            const float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
            if (mcui::Button(L("Другая…", "Change…"), mcui::ButtonKind::Default, ImVec2(half, 0)))
                m_openSurface = true, m_surfaceAt = ImGui::GetItemRectMin();
            ImGui::SameLine();
            if (mcui::Button(L("Убрать", "Remove"), mcui::ButtonKind::Default, ImVec2(half, 0)))
                pushUndo(&doc), s.material = port::platform::withSurface(s.material, 0);
        }
        ImGui::EndDisabled();
    }
    void surfaceChip() {
        if (sel.kind != Kind::Shape || !selValid(doc.map, sel) || !m_group.empty() || m_dragging || m_resize || m_areaDrag || m_paletteIds.empty() ||
            doc.map.shapes[size_t(sel.index)].layer != 0)
            return;
        const Shape& s = doc.map.shapes[size_t(sel.index)];
        const float sc = m_scale, pad = 3 * sc, size = (m_touchSeen ? 44 : 32) * sc, outer = size + 2 * pad, inset = 14 * sc;
        const ImVec2 lo = view.toScreen(toF(s.box[0]), toF(s.box[3])), hi = view.toScreen(toF(s.box[2]), toF(s.box[1]));
        const bool inside = s.type == 0 && hi.x - lo.x > 3 * outer && hi.y - lo.y > 2 * outer;
        const ImVec2 at = inside ? ImVec2(hi.x - inset - outer, lo.y + inset) : ImVec2(hi.x + 12 * sc, lo.y);
        if (at.x < view.origin.x || at.y < view.origin.y || at.x + outer > view.origin.x + view.size.x || at.y + outer > view.origin.y + view.size.y)
            return;
        if (at.x < m_inspectorMax.x && at.x + outer > m_inspectorMin.x && at.y < m_inspectorMax.y && at.y + outer > m_inspectorMin.y) return;
        beginFloat("##surfacechip", at, ImVec2(0, 0), 0, 3);
        const int id = port::platform::shapeSurface(s.material);
        SDL_Texture* tex = nullptr;
        ImVec2 sz;
        const char* tip = L("Поверхность — кусок палитры на всю форму", "Surface — a palette piece over the whole shape");
        bool pressed;
        if (id && frontTexture(id, &tex, &sz)) {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            pressed = ImGui::InvisibleButton("##surface", ImVec2(size, size));
            ImDrawList* dl = ImGui::GetWindowDrawList();
            if (ImGui::IsItemHovered()) dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size), tok(mcui::tokens::kColorBgHover), 6 * sc);
            dl->AddImageRounded(ImTextureRef(ImTextureID(tex)), ImVec2(p.x + 2 * sc, p.y + 2 * sc), ImVec2(p.x + size - 2 * sc, p.y + size - 2 * sc),
                                ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 5 * sc);
            if (ImGui::IsItemHovered() && !m_touchSeen) ImGui::SetTooltip("%s", tip);
        } else {
            pressed = iconButton("##surface", icons().piece, size, tok(mcui::tokens::kColorTextSecondary), tok(mcui::tokens::kColorBgSecondary), tip);
        }
        if (pressed) m_openSurface = true, m_surfaceAt = ImVec2(at.x, at.y + outer + 6 * sc);
        endFloat();
    }
    void surfacePopup() {
        const float sc = m_scale, w = 300 * sc, hMax = 520 * sc;
        if (m_openSurface) {
            m_openSurface = false;
            ImGui::OpenPopup("##surface");
            const float x = std::max(view.origin.x, std::min(m_surfaceAt.x, view.origin.x + view.size.x - w));
            const float y = std::max(view.origin.y, std::min(m_surfaceAt.y, view.origin.y + view.size.y - hMax));
            ImGui::SetNextWindowPos(ImVec2(x, y));
        }
        ImGui::SetNextWindowSizeConstraints(ImVec2(w, 0), ImVec2(w, hMax));
        if (!ImGui::BeginPopup("##surface")) return;
        if (sel.kind != Kind::Shape || !selValid(doc.map, sel)) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        Shape& shape = doc.map.shapes[size_t(sel.index)];
        ImGui::PushFont(mcui::GetFonts().bold, 0.0f);
        ImGui::TextUnformatted(L("Поверхность", "Surface"));
        ImGui::PopFont();
        caption(L("Фасад куска ляжет копиями на всю лицевую сторону, его крыша — по верху (у рампы — по скосу), по краям "
                  "копии обрезаются. Подойдут дома и фасады с плоской крышей.",
                  "The piece's facade is repeated over the whole front, its roof over the top (a ramp's slope), cut at the "
                  "edges. Buildings and facades with a flat roof fit."));
        previewsNote(true);
        const int current = port::platform::shapeSurface(shape.material);
        if (current && mcui::Button(L("Без поверхности", "No surface"), mcui::ButtonKind::Default, ImVec2(-FLT_MIN, 0))) {
            pushUndo(&doc);
            shape.material = port::platform::withSurface(shape.material, 0);
            ImGui::CloseCurrentPopup();
        }
        const int picked = pieceGrid(current, true);
        if (picked >= 0 && picked <= port::platform::kMaxSurfacePiece) {
            pushUndo(&doc);
            shape.material = port::platform::withSurface(shape.material, picked);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    void previewProgress(float width) {
        const float fraction = m_progressTotal > 0 ? float(m_progressDone) / float(m_progressTotal) : 0.0f;
        char text[96];
        if (m_progressTotal > 0) snprintf(text, sizeof text, L("картинки кусков: %d из %d", "piece previews: %d of %d"), m_progressDone, m_progressTotal);
        else snprintf(text, sizeof text, "%s", L("картинки кусков готовятся…", "piece previews are being made…"));
        ImGui::PushFont(nullptr, mcui::kTextCaption);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, mcui::Color(mcui::tokens::kColorAccent));
        ImGui::ProgressBar(fraction, ImVec2(width, 0), text);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    void previewsNote(bool facades) {
        if (m_piecesChild) {
            caption(L("Картинки кусков этой главы ещё рисуются — плитки появятся сами, когда будут готовы.",
                      "The chapter's piece previews are still being made — the tiles show up by themselves when they are ready."));
            previewProgress(ImGui::GetContentRegionAvail().x);
            return;
        }
#ifdef MEDECOMP_ONE_PROCESS
        if (!m_piecesEnv.empty()) {
            if (mcui::Button(L("Подготовить картинки кусков", "Prepare piece previews"), mcui::ButtonKind::Accent, ImVec2(-FLT_MIN, 0)))
                requestService(m_piecesEnv, "", "pieces");
            caption(L("Картинки рисует игра: редактор закроется на время и вернётся к этому уровню",
                      "The game draws them: the editor closes for a while and comes back to this level"));
            return;
        }
#endif
        if (!facades) return;
        for (auto& kv : m_paletteIds) {
            auto face = m_pieceFace.find(kv.first);
            if (face != m_pieceFace.end() && face->second.front) return;
        }
        caption(m_pieceFace.empty() ? L("Картинки кусков нарисованы не до конца — откройте уровень заново, и они нарисуются снова.",
                                        "The piece previews were not finished — open the level again and they are made anew.")
                                    : L("У кусков этой главы нет фасада: поверхность из них не сложить.",
                                        "No piece of this chapter has a facade: none of them makes a surface."));
    }

    void beginFloat(const char* id, ImVec2 pos, ImVec2 pivot, float width, float pad, float height = 0, bool through = false, float maxHeight = 0) {
        const float s = m_scale;
        const bool compact = compactMobile();
        const float edge = 16 * s, maxW = std::max(1.0f, view.size.x - 2 * edge), maxH = std::max(1.0f, std::min(view.size.y - 2 * edge, maxHeight > 0 ? maxHeight : FLT_MAX));
        if (compact) {
            width = width > 0 ? std::min(width, maxW) : 0;
            height = height > 0 ? std::min(height, maxH) : 0;
            ImGuiWindow* previous = ImGui::FindWindowByName(id);
            const ImVec2 estimate(width > 0 ? width : previous ? std::min(previous->Size.x, maxW) : maxW,
                                   height > 0 ? height : previous ? std::min(previous->Size.y, maxH) : 40 * s);
            pos.x -= pivot.x * estimate.x;
            pos.y -= pivot.y * estimate.y;
            pos.x = std::max(view.origin.x + edge, std::min(view.origin.x + view.size.x - edge - estimate.x, pos.x));
            pos.y = std::max(view.origin.y + edge, std::min(view.origin.y + view.size.y - edge - estimate.y, pos.y));
            pivot = ImVec2(0, 0);
        }
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
        if (height > 0) ImGui::SetNextWindowSize(ImVec2(width, height));
        else if (width > 0 || compact) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0), ImVec2(width > 0 ? width : maxW, compact ? maxH : FLT_MAX),
                compact ? +[](ImGuiSizeCallbackData* data) { data->DesiredSize.y = ceilf(data->DesiredSize.y) + 1.0f; } : nullptr);
        }
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18 * s);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad * s, pad * s));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, mcui::Color(mcui::tokens::kColorBgSecondary, 0.97f));
        ImGui::PushStyleColor(ImGuiCol_Border, mcui::Color(mcui::tokens::kColorBgTertiary));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoFocusOnAppearing;
        if (height <= 0) flags |= ImGuiWindowFlags_AlwaysAutoResize;
        if (height > 0 || compact) flags &= ~ImGuiWindowFlags_NoScrollbar;
        if (through) flags |= ImGuiWindowFlags_NoInputs;
        ImGui::Begin(id, nullptr, flags);
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        if (compact && !through) port::platform::panelTouchScroll(s);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * s, 8 * s));
    }
    void endFloat() {
        ImGui::PopStyleVar();
        ImGui::End();
    }
    void fieldLabel(const char* text) {
        ImGui::PushFont(nullptr, mcui::kTextCaption);
        ImGui::TextDisabled("%s", text);
        ImGui::PopFont();
    }
    void caption(const char* text) {
        ImGui::PushFont(nullptr, mcui::kTextCaption);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", text);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    void levelCard() {
        const float s = m_scale;
        beginFloat("##level", ImVec2(view.origin.x + 16 * s, view.origin.y + 16 * s), ImVec2(0, 0), 288 * s, 16);
        const Icons& ic = icons();
        ImGui::AlignTextToFramePadding();
        ImGui::PushFont(mcui::GetFonts().bold, 18.0f);
        ImGui::TextUnformatted(doc.id.c_str());
        ImGui::PopFont();
        ImGui::SameLine(ImGui::GetContentRegionMax().x - 28 * s);
        if (iconButton("##fold", m_levelOpen ? ic.up : ic.down, 28 * s, tok(mcui::tokens::kColorTextMuted), 0,
                       m_levelOpen ? L("Свернуть", "Collapse") : L("Развернуть", "Expand")))
            m_levelOpen = !m_levelOpen;
        if (m_levelOpen) {
            const bool arena = port::platform::arenaBase(doc.palette);
            std::string place = std::string(L("на месте: ", "in place of: ")) + baseTitle(doc.palette) + " · " +
                                (doc.origin == "empty"      ? L("с нуля — можно выложить", "from scratch — can be uploaded")
                                 : doc.origin == "uploaded" ? L("выложен — новая версия по кнопке в игре", "uploaded — a new version from the game")
                                 : doc.origin == "online"   ? L("скачан", "downloaded")
                                 : arena                    ? L("копия арены — только у себя", "an arena copy — stays on this PC")
                                                            : L("копия главы — только у себя", "a chapter copy — stays on this PC"));
            caption(place.c_str());
            char buf[256];
            fieldLabel(L("Название", "Name"));
            snprintf(buf, sizeof buf, "%s", doc.name.c_str());
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##name", buf, sizeof buf)) doc.name = buf;
            fieldLabel(L("Автор", "Author"));
            snprintf(buf, sizeof buf, "%s", doc.author.c_str());
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##author", buf, sizeof buf)) doc.author = buf;
            fieldLabel(L("Режим", "Mode"));
            const char* modes[] = {L("Гонка", "Race"), L("Сбор сумок", "Bag collection")};
            int mode = doc.bags ? 1 : 0;
            ImGui::BeginDisabled(arena);
            if (mcui::Segmented("##mode", modes, 2, &mode)) doc.bags = mode == 1;
            ImGui::EndDisabled();
            if (arena) caption(L("на арене — только сбор сумок", "an arena takes bag collection only"));
            if (doc.bags) {
                fieldLabel(L("Время", "Time limit"));
                char names[3][24];
                const char* items[3];
                int current = 0;
                for (int i = 0; i < 3; i++) {
                    snprintf(names[i], sizeof names[i], "%d %s", port::platform::kBagTimeLimits[i] / 60000, L("мин", "min"));
                    items[i] = names[i];
                    if (doc.timeLimit == port::platform::kBagTimeLimits[i]) current = i;
                }
                if (mcui::Segmented("##limit", items, 3, &current)) doc.timeLimit = port::platform::kBagTimeLimits[current];
            }
            bool supports = supportsShown();
            if (mcui::Toggle(L("Видимые коробки и рампы", "Visible blocks and ramps"), &supports))
                doc.supports = supports ? "visible" : "hidden";
            caption(L("В игре — белыми фигурами. Выключите, если закрыли их кусками палитры.",
                      "In the game as white shapes. Turn off if palette pieces cover them."));
            caption(doc.bags ? L("Нужны сумки; финиш не нужен — забег идёт, пока не выйдет время или не собраны все сумки.",
                                 "Needs bags; no finish needed — the run goes on until the time is up or every bag is collected.")
                             : L("Нужны старт и финиш; чекпоинты — по желанию.", "Needs a start and a finish; checkpoints are optional."));
        }
        m_levelBottom = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
        endFloat();
    }

    void actionsBar() {
        const float s = m_scale, h = 36 * s;
        const bool stacked = compactMobile() && view.size.x < 560 * s;
        const float top = stacked ? m_levelBottom + 8 * s : view.origin.y + 16 * s;
        beginFloat("##actions", ImVec2(view.origin.x + view.size.x - 16 * s, top), ImVec2(1, 0), 0, 6);
        ImGui::BeginDisabled(doc.undo.empty());
        if (iconButton("##undo", icons().undo, h, tok(mcui::tokens::kColorTextSecondary), 0, L("Отменить · Ctrl+Z", "Undo · Ctrl+Z"))) undo();
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (mcui::Button(L("Сохранить", "Save"), mcui::ButtonKind::Default, ImVec2(0, h))) doSave();
        if (!m_touchSeen && ImGui::IsItemHovered()) ImGui::SetTooltip("Ctrl+S");
        ImGui::SameLine();
        if (mcui::Button(L("Проверить", "Test"), mcui::ButtonKind::Accent, ImVec2(0, h))) test();
        if (!m_touchSeen && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", L("F5 — сохранить и запустить в игре", "F5 — save and run in the game"));
        m_actionsBottom = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
        endFloat();
        if (doc.status.empty() || doc.status == m_statusClosed) return;
        beginFloat("##status", ImVec2(view.origin.x + view.size.x - 16 * s, m_actionsBottom + 8 * s), ImVec2(1, 0), 0, 10);
        const float statusWidth = compactMobile() ? std::min(320 * s, std::max(1.0f, view.size.x - 82 * s - ImGui::GetStyle().ItemSpacing.x)) : 320 * s;
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + statusWidth);
        ImGui::TextUnformatted(doc.status.c_str());
        ImGui::PopTextWrapPos();
        ImGui::SameLine();
        if (iconButton("##hide", icons().close, 22 * s, tok(mcui::tokens::kColorTextMuted))) m_statusClosed = doc.status;
        endFloat();
    }

    Node& selNode(Map& m) { return m.layers[size_t(sel.layer)].columns[size_t(sel.column)].sections[size_t(sel.section)].nodes[size_t(sel.index)]; }

    void selBounds(ImVec2* lo, ImVec2* hi) {
        boundsOf(sel, lo, hi);
        for (const Sel& g : selection()) {
            ImVec2 a, b;
            boundsOf(g, &a, &b);
            lo->x = std::min(lo->x, a.x), lo->y = std::min(lo->y, a.y), hi->x = std::max(hi->x, b.x), hi->y = std::max(hi->y, b.y);
        }
    }
    void boundsOf(const Sel& s, ImVec2* lo, ImVec2* hi) {
        Map& m = doc.map;
        const Fix* p = posOf(m, s);
        float ax = toF(p[0]), ay = toF(p[1]), bx = ax, by = ay;
        switch (s.kind) {
        case Kind::Shape:
        case Kind::Checkpoint: bx = toF(p[2]), by = toF(p[3]); break;
        case Kind::Finish: bx = ax + toF(p[2]), by = ay + toF(p[3]); break;
        case Kind::Node: {
            std::array<float, 6> b;
            const Node& n = m.layers[size_t(s.layer)].columns[size_t(s.column)].sections[size_t(s.section)].nodes[size_t(s.index)];
            if (nodeBox(n, &b)) bx = ax + b[3], by = ay + b[4], ax += b[0], ay += b[1];
            break;
        }
        default: break;
        }
        ImVec2 a = view.toScreen(ax, by), c = view.toScreen(bx, ay);
        const float r = 8 * m_scale;
        *lo = ImVec2(std::min(a.x, c.x) - r, std::min(a.y, c.y) - r);
        *hi = ImVec2(std::max(a.x, c.x) + r, std::max(a.y, c.y) + r);
    }

    std::string selTitle() {
        Map& m = doc.map;
        switch (sel.kind) {
        case Kind::Shape: {
            const char* names[] = {L("Коробка", "Block"), L("Рампа к −x", "Ramp to −x"), L("Рампа к +x", "Ramp to +x")};
            return names[std::max(0, std::min(2, int(m.shapes[size_t(sel.index)].type)))];
        }
        case Kind::Node: {
            const Node& n = selNode(m);
            return std::string(L("Кусок ", "Piece ")) + std::to_string(n.id) + (n.visionFlag == 1 ? L(" · подсветка", " · highlight") : "");
        }
        case Kind::Object: {
            int type = m.objects[size_t(sel.index)].type;
            return type == kBag     ? L("Сумка", "Bag")
                   : type == kSwing ? L("Турник", "Swing bar")
                                    : std::string(L("Объект: ", "Object: ")) + objectName(type);
        }
        case Kind::Start: return L("Старт", "Start");
        case Kind::Finish: return L("Финиш", "Finish");
        case Kind::Checkpoint: return L("Чекпоинт", "Checkpoint");
        case Kind::Sound: {
            const char* names[] = {L("Фоновый звук", "Ambient sound"), L("Разовый звук", "One-shot sound"), L("Музыкальная метка", "Music cue")};
            return names[std::max(0, std::min(2, sel.list))];
        }
        case Kind::ZipStart: return L("Трос · начало", "Zip line · start");
        case Kind::ZipEnd: return L("Трос · конец", "Zip line · end");
        default: return "";
        }
    }
    ImU32 selColor() {
        if (sel.kind == Kind::Object && doc.map.objects[size_t(sel.index)].type == kBag) return tok(mcui::tokens::kColorAttention);
        if (sel.kind == Kind::Start) return tok(mcui::tokens::kColorPositive);
        if (sel.kind == Kind::Checkpoint) return kAccentLight;
        return tok(mcui::tokens::kColorTextPrimary);
    }
    void selPreview() {
        Map& m = doc.map;
        int id = -1;
        if (sel.kind == Kind::Node) id = selNode(m).id;
        if (sel.kind == Kind::Object && m.objects[size_t(sel.index)].type == kBag) id = kBagModel;
        if (sel.kind == Kind::Checkpoint) {
            const Ref& r = m.checkpoints[size_t(sel.index)].ref;
            Sel at;
            at.kind = Kind::Node, at.layer = r.layer, at.column = r.column, at.section = r.section, at.index = r.node;
            if (selValid(m, at))
                id = m.layers[size_t(r.layer)].columns[size_t(r.column)].sections[size_t(r.section)].nodes[size_t(r.node)].id;
        }
        if (id >= 0) pieceImage(id);
    }

    int edgesAt(ImVec2 at) {
        float b[4];
        if (!m_group.empty() || !selValid(doc.map, sel) || !selBox(doc.map, sel, b)) return 0;
        const ImVec2 lo = view.toScreen(b[0], b[3]), hi = view.toScreen(b[2], b[1]);
        const float gx = std::min(6 * m_scale, (hi.x - lo.x) / 4), gy = std::min(6 * m_scale, (hi.y - lo.y) / 4);
        if (at.x < lo.x - 6 * m_scale || at.x > hi.x + 6 * m_scale || at.y < lo.y - 6 * m_scale || at.y > hi.y + 6 * m_scale) return 0;
        int e = 0;
        if (at.x <= lo.x + gx) e |= EdgeLeft;
        else if (at.x >= hi.x - gx) e |= EdgeRight;
        if (at.y <= lo.y + gy) e |= EdgeTop;
        else if (at.y >= hi.y - gy) e |= EdgeBottom;
        if ((at.x < lo.x || at.x > hi.x) && !(e & (EdgeLeft | EdgeRight))) return 0;
        if ((at.y < lo.y || at.y > hi.y) && !(e & (EdgeTop | EdgeBottom))) return 0;
        return e;
    }
    void handles(ImDrawList* dl) {
        float b[4];
        if (!m_group.empty() || !selValid(doc.map, sel) || !selBox(doc.map, sel, b)) return;
        const ImVec2 lo = view.toScreen(b[0], b[3]), hi = view.toScreen(b[2], b[1]);
        const float r = 3.5f * m_scale, xs[3] = {lo.x, (lo.x + hi.x) / 2, hi.x}, ys[3] = {lo.y, (lo.y + hi.y) / 2, hi.y};
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                if (i == 1 && j == 1) continue;
                dl->AddRectFilled(ImVec2(xs[i] - r, ys[j] - r), ImVec2(xs[i] + r, ys[j] + r), IM_COL32(255, 255, 255, 255));
                dl->AddRect(ImVec2(xs[i] - r, ys[j] - r), ImVec2(xs[i] + r, ys[j] + r), IM_COL32(20, 22, 28, 255));
            }
    }

    void inspector() {
        Map& m = doc.map;
        if (!selValid(m, sel)) return;
        const float s = m_scale, w = 312 * s, gap = 14 * s, edge = 16 * s, h = m_inspectorH;
        ImVec2 lo, hi;
        selBounds(&lo, &hi);
        const float x0 = view.origin.x + edge;
        const float y0 = compactMobile() ? std::max(m_levelBottom, m_actionsBottom) + 8 * s : view.origin.y + edge;
        const float x1 = std::max(x0, view.origin.x + view.size.x - edge - w),
                    y1 = std::max(y0, view.origin.y + view.size.y - edge - h - depthReserve());
        const float cx = (lo.x + hi.x - w) / 2, cy = (lo.y + hi.y - h) / 2;
        const ImVec2 spots[4] = {ImVec2(cx, lo.y - gap - h), ImVec2(cx, hi.y + gap), ImVec2(hi.x + gap, cy), ImVec2(lo.x - gap - w, cy)};
        ImVec2 pos;
        float least = -1;
        for (ImVec2 p : spots) {
            p.x = std::max(x0, std::min(x1, p.x)), p.y = std::max(y0, std::min(y1, p.y));
            const float cover = std::max(0.0f, std::min(p.x + w, hi.x) - std::max(p.x, lo.x)) * std::max(0.0f, std::min(p.y + h, hi.y) - std::max(p.y, lo.y));
            if (least < 0 || cover < least - 1) least = cover, pos = p;
        }
        const bool ghost = ImGui::GetIO().KeyCtrl && !ImGui::GetIO().WantTextInput;
        if (ghost) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.25f);
        const float maxHeight = compactMobile() ? std::max(1.0f, view.origin.y + view.size.y - viewBarReserve() - y0) : 0;
        beginFloat("##inspector", pos, ImVec2(0, 0), w, 14, 0, ghost, maxHeight);
        inspectorBody();
        m_inspectorH = ImGui::GetWindowSize().y;
        m_inspectorMin = ImGui::GetWindowPos();
        m_inspectorMax = ImVec2(m_inspectorMin.x + ImGui::GetWindowSize().x, m_inspectorMin.y + m_inspectorH);
        endFloat();
        if (ghost) ImGui::PopStyleVar();
    }
    void inspectorBody() {
        Map& m = doc.map;
        const float s = m_scale;
        if (!m_group.empty()) {
            ImGui::AlignTextToFramePadding();
            ImGui::PushFont(mcui::GetFonts().bold, 0.0f);
            ImGui::Text(L("Выбрано: %d", "Selected: %d"), int(selection().size()));
            ImGui::PopFont();
            ImGui::SameLine(ImGui::GetContentRegionMax().x - 28 * s);
            if (iconButton("##delete", icons().trash, 28 * s, kDanger, 0, L("Удалить все · Del", "Delete all · Del"))) deleteSelected();
            caption(groupSummary().c_str());
            if (m_touchSeen) caption(L("Тяните любой объект — сдвинется вся группа. Коснитесь объекта, чтобы выбрать его одного. Кнопка ниже снимает выделение.",
                                      "Drag any object to move the group. Tap an object to select it alone. Use the button below to clear the selection."));
            else caption(L("Тяните любое из них — сдвинется вся группа. Ctrl+щелчок — добавить или убрать, щелчок по одному — выбрать "
                      "только его, Esc — снять.",
                      "Drag any of them — the whole group moves. Ctrl+click — add or remove, click one — select only it, Esc — "
                      "clear."));
            if (mcui::Button(L("Снять выделение", "Clear the selection"), mcui::ButtonKind::Default, ImVec2(-FLT_MIN, 0)))
                sel = Sel(), m_group.clear(), m_groupFor = Sel();
            return;
        }
        ImGui::AlignTextToFramePadding();
        ImGui::PushFont(mcui::GetFonts().bold, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(selColor()));
        ImGui::TextUnformatted(selTitle().c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SameLine(ImGui::GetContentRegionMax().x - 28 * s);
        if (iconButton("##delete", icons().trash, 28 * s, kDanger, 0, L("Удалить · Del", "Delete · Del"))) deleteSelected();
        if (sameSel(sel, m_stackSel) && m_stackCount > 1) {
            char text[160];
            snprintf(text, sizeof text, L("Здесь %d одно за другим, это — %d-е. Щёлкните ещё раз — выберется следующее.",
                                          "%d here, one behind another; this is #%d. Click again for the next one."),
                     m_stackCount, m_stackPos + 1);
            caption(text);
        }
        if (selValid(m, sel)) {
            selPreview();
            properties();
            coverButton();
        }
    }

    void checkpointDish(int i) {
        Map& m = doc.map;
        const Checkpoint& c = m.checkpoints[size_t(i)];
        Node* d = refNode(m, c.ref);
        if (!d) return caption(L("Тарелки нет: ссылка не на кусок.", "No dish: the reference is not a piece."));
        const float x0 = toF(c.box[0]), y0 = toF(c.box[1]), x1 = toF(c.box[2]), y1 = toF(c.box[3]);
        const float dx = toF(d->pos[0]), dy = toF(d->pos[1]);
        const bool nearby = dx >= x0 - 3 && dx <= x1 + 3 && dy >= y0 - 3 && dy <= y1;
        const int users = refCount(m, c.ref);
        char text[200];
        if (nearby && users == 1)
            snprintf(text, sizeof text, "%s", L("Тарелка рядом: повернётся, когда Фейт пробежит чекпоинт, — тут она и возродится.",
                                                "The dish is next to it: it turns when Faith runs through, and she respawns here."));
        else if (users > 1)
            snprintf(text, sizeof text, L("Поворачивает тарелку у x %.1f — общую с другим чекпоинтом (так раньше ставились новые чекпоинты).",
                                          "Turns the dish at x %.1f — shared with another checkpoint (new checkpoints used to be set so)."), dx);
        else
            snprintf(text, sizeof text, L("Поворачивает тарелку у x %.1f — далеко от себя.", "Turns the dish at x %.1f — far from it."), dx);
        caption(text);
        if (!(nearby && users == 1)) {
            if (mcui::Button(L("Своя тарелка здесь", "Own dish here"), mcui::ButtonKind::Accent, ImVec2(-FLT_MIN, 0))) ownDish(i);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", L("такой же кусок рядом станет ею, иначе — новая у угла чекпоинта",
                                          "the same piece nearby becomes it, otherwise a new one at the checkpoint's corner"));
            return;
        }
        bool right = d->visionType != 1;
        if (ImGui::Checkbox(L("после возрождения — лицом вправо", "respawn facing right"), &right)) pushUndo(&doc), d->visionType = int8_t(right ? 2 : 1);
    }

    void coverButton() {
        if (sel.kind != Kind::Shape || !selValid(doc.map, sel) || doc.map.shapes[size_t(sel.index)].layer != 0) return;
        const bool box = doc.map.shapes[size_t(sel.index)].type == 0;
        ImGui::BeginDisabled(!box || m_paletteIds.empty());
        if (mcui::Button(L("Покрыть куском…", "Cover with a piece…"), mcui::ButtonKind::Accent, ImVec2(-FLT_MIN, 0)) || m_openCover)
            m_openCover = false, ImGui::OpenPopup("##cover");
        ImGui::EndDisabled();
        if (!box && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", L("только коробку: на скос рампы плоские куски не лягут", "boxes only: flat pieces do not fit a ramp's slope"));
        const float s = m_scale;
        ImGui::SetNextWindowSizeConstraints(ImVec2(300 * s, 0), ImVec2(300 * s, 520 * s));
        if (ImGui::BeginPopup("##cover")) {
            ImGui::PushFont(mcui::GetFonts().bold, 0.0f);
            ImGui::TextUnformatted(L("Покрыть куском", "Cover with a piece"));
            ImGui::PopFont();
            caption(L("Копии куска лягут рядом по верху коробки на всю её длину. Подойдут пол, крыша, карниз.",
                      "Copies of the piece go in a row along the top of the block, its whole length. Floors, roofs, ledges fit."));
            previewsNote(false);
            int picked = pieceGrid();
            if (picked >= 0) {
                coverShape(picked);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void areaBar() {
        if (!m_hasArea || m_areaDrag) return;
        const float s = m_scale;
        float a[4];
        area(a);
        ImVec2 top = view.toScreen((a[0] + a[2]) / 2, a[3]), bottom = view.toScreen((a[0] + a[2]) / 2, a[1]);
        const bool above = top.y - 60 * s > view.origin.y + 16 * s;
        const float width = compactMobile() ? std::min(650 * s, std::max(1.0f, view.size.x - 32 * s)) : 0;
        beginFloat("##area", above ? ImVec2(top.x, top.y - 12 * s) : ImVec2(bottom.x, bottom.y + 12 * s), ImVec2(0.5f, above ? 1.0f : 0.0f), width, 8);
        auto next = [&](const char* label, float fixedWidth = 0) {
            const float needed = fixedWidth > 0 ? fixedWidth : ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2 + 4 * s;
            if (!compactMobile() || ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + needed <= ImGui::GetCurrentWindow()->WorkRect.Max.x)
                ImGui::SameLine();
        };
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%.2f × %.2f", a[2] - a[0], a[3] - a[1]);
        next(L("Заполнить выбранным", "Fill with selected"));
        ImGui::BeginDisabled(sel.kind != Kind::Shape && sel.kind != Kind::Node);
        if (mcui::Button(L("Заполнить выбранным", "Fill with selected"))) fillArea();
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", L("копии выбранной коробки или куска — шагом их размера", "copies of the selected block or piece, at their size"));
        next(L("Удалить тайлы", "Remove tiles"));
        if (mcui::Button(L("Удалить тайлы", "Remove tiles"))) clearArea();
        next(L("Выделить всё внутри", "Select all inside"));
        if (mcui::Button(L("Выделить всё внутри", "Select all inside"))) selectInArea();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", L("коробки, рампы, куски и объекты целиком в области — тянуть группой",
                                      "blocks, ramps, pieces and objects wholly inside — to drag as a group"));
        next(L("Как в игре", "As in the game"));
        ImGui::BeginDisabled(m_mapChild != 0);
        if (mcui::Button(L("Как в игре", "As in the game"))) previewArea();
        ImGui::EndDisabled();
        next(nullptr, ImGui::GetFrameHeight());
        if (iconButton("##clear", icons().close, ImGui::GetFrameHeight(), tok(mcui::tokens::kColorTextMuted), 0, L("Снять область", "Clear the area")))
            m_hasArea = false;
        if (m_mapChild) caption(L("игра рисует область…", "the game is drawing the area…"));
#ifdef MEDECOMP_ONE_PROCESS
        caption(L("«Как в игре»: область рисует игра — редактор закроется и вернётся к этому месту",
                  "As in the game: the game draws the area — the editor closes and comes back here"));
#endif
        endFloat();
    }

    void zoomBy(float k) { view.zoom = std::max(0.2f, std::min(200.0f, view.zoom * k)); }

    void viewBar() {
        const float s = m_scale, h = 32 * s, gap = ImGui::GetStyle().ItemSpacing.x;
        auto buttonWidth = [&](const char* label) {
            ImGui::PushFont(mcui::GetFonts().medium, 0.0f);
            const float width = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2 + 2 * s;
            ImGui::PopFont();
            return width;
        };
        const std::string percent = std::to_string(int(roundf(100 * view.zoom / (8 * m_scale)))) + "%";
        const float zoomWidth = h * 2 + ImGui::CalcTextSize(percent.c_str()).x + buttonWidth(L("Вписать", "Fit")) + gap * 3;
        const float fullWidth = buttonWidth(L("Формы", "Shapes")) + buttonWidth(L("Графика", "Graphics")) +
                                buttonWidth(L("Объекты", "Objects")) + buttonWidth(L("Сверху", "Top")) +
                                zoomWidth + 4 * s + gap * 5 + 12 * s;
        const float width = compactMobile() ? std::min(fullWidth, std::max(1.0f, view.size.x - 32 * s)) : 0;
        beginFloat("##viewbar", ImVec2(view.origin.x + view.size.x / 2, view.origin.y + view.size.y - 16 * s), ImVec2(0.5f, 1.0f), width, 6);
        bool hasItem = false;
        auto pill = [&](const char* label, bool* on) {
            if (hasItem && (!compactMobile() || ImGui::GetItemRectMax().x + gap + buttonWidth(label) <= ImGui::GetCurrentWindow()->WorkRect.Max.x))
                ImGui::SameLine();
            if (mcui::Button(label, *on ? mcui::ButtonKind::Accent : mcui::ButtonKind::Default, ImVec2(0, h))) *on = !*on;
            hasItem = true;
        };
        pill(L("Формы", "Shapes"), &showShapes);
        pill(L("Графика", "Graphics"), &showGraphics);
        pill(L("Объекты", "Objects"), &showObjects);
        pill(L("Сверху", "Top"), &showDepth);
        if (compactMobile() && ImGui::GetItemRectMax().x + gap * 2 + 4 * s + zoomWidth > ImGui::GetCurrentWindow()->WorkRect.Max.x) {
            ImGui::Spacing();
        } else {
            ImGui::SameLine();
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + 2 * s, p.y + 6 * s), ImVec2(p.x + 2 * s, p.y + h - 6 * s), tok(mcui::tokens::kColorBgHover));
            ImGui::Dummy(ImVec2(4 * s, h));
            ImGui::SameLine();
        }
        if (mcui::Button("−", mcui::ButtonKind::Ghost, ImVec2(h, h))) zoomBy(1 / 1.25f);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(percent.c_str());
        ImGui::SameLine();
        if (mcui::Button("+", mcui::ButtonKind::Ghost, ImVec2(h, h))) zoomBy(1.25f);
        ImGui::SameLine();
        if (mcui::Button(L("Вписать", "Fit"), mcui::ButtonKind::Default, ImVec2(0, h))) fit();
        if (m_piecesChild) {
            if (!compactMobile() || ImGui::GetItemRectMax().x + gap + 200 * s <= ImGui::GetCurrentWindow()->WorkRect.Max.x) ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (h - ImGui::GetFrameHeight()) / 2);
            previewProgress(200 * s);
        }
        m_viewBarW = ImGui::GetWindowSize().x;
        m_viewBarH = ImGui::GetWindowSize().y;
        endFloat();
    }

    void piecesFlyout() {
        const float s = m_scale, edge = 16 * s;
        const float panelBottom = compactMobile() && view.size.x < 560 * s ? std::max(m_levelBottom, m_actionsBottom) : m_levelBottom;
        const float top = std::max(view.origin.y + edge, panelBottom) + 12 * s;
        const float h = view.origin.y + view.size.y - edge - (compactMobile() ? viewBarReserve() : 64 * s) - top;
        if (h < 140 * s) return;
        beginFloat("##pieces", ImVec2(view.origin.x + edge, top), ImVec2(0, 0), kPaletteWidth * s, 14, h);
        if (!compactMobile()) port::platform::panelTouchScroll(m_scale);
        ImGui::PushFont(mcui::GetFonts().bold, 0.0f);
        ImGui::TextUnformatted(L("Куски палитры", "Palette pieces"));
        ImGui::PopFont();
        caption((std::string(L("глава ", "chapter ")) + chapterTitle(doc.palette) +
                 L(" · выберите кусок, потом щелчок по полю", " · pick a piece, then click the map"))
                    .c_str());
        if (m_piecesChild) previewProgress(ImGui::GetContentRegionAvail().x);
#ifdef MEDECOMP_ONE_PROCESS
        if (!m_piecesEnv.empty()) {
            if (mcui::Button(L("Подготовить картинки кусков", "Prepare piece previews"))) requestService(m_piecesEnv, "", "pieces");
            caption(L("Картинки рисует игра: редактор закроется на время и вернётся к этому уровню",
                      "The game draws them: the editor closes for a while and comes back to this level"));
        }
#endif
        int picked = pieceGrid();
        if (picked >= 0) m_piece = picked;
        endFloat();
    }

    int pieceGrid(int chosenId = -1, bool facades = false) {
        const float s = m_scale;
        int picked = -1;
        std::vector<std::pair<int, int>> order;
        for (auto& kv : m_paletteIds) {
            auto face = m_pieceFace.find(kv.first);
            if (facades && (face == m_pieceFace.end() || !face->second.front)) continue;
            order.push_back({kv.second.first, kv.first});
        }
        std::sort(order.begin(), order.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
            return a.first != b.first ? a.first > b.first : a.second < b.second;
        });
        const int marked = chosenId >= 0 ? chosenId : m_piece;
        if (m_pieceGroup.empty()) return pieceTiles(order, marked);
        std::vector<std::pair<int, int>> groups[kGroupCount];
        for (const auto& o : order) {
            auto g = m_pieceGroup.find(o.second);
            groups[g == m_pieceGroup.end() ? kGroupOther : g->second].push_back(o);
        }
        for (int g = 0; g < kGroupCount; g++) {
            if (groups[g].empty()) continue;
            const bool hasMarked = std::any_of(groups[g].begin(), groups[g].end(), [&](const std::pair<int, int>& o) { return o.second == marked; });
            if (hasMarked) ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            char label[160];
            snprintf(label, sizeof label, "%s · %zu##group%d", paletteGroupName(g), groups[g].size(), g);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8 * s, ImGui::GetStyle().FramePadding.y));
            const bool open = ImGui::CollapsingHeader(label);
            ImGui::PopStyleVar();
            if (!open) continue;
            ImGui::PushID(g);
            const int got = pieceTiles(groups[g], marked);
            ImGui::PopID();
            if (got >= 0) picked = got;
        }
        return picked;
    }
    int pieceTiles(const std::vector<std::pair<int, int>>& order, int marked) {
        const float s = m_scale;
        int picked = -1;
        const float gap = 8 * s, tile = (ImGui::GetContentRegionAvail().x - gap) / 2;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (size_t i = 0; i < order.size(); i++) {
            const int id = order[i].second;
            ImGui::PushID(id);
            if (i % 2) ImGui::SameLine(0, gap);
            ImVec2 p = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("##piece", ImVec2(tile, tile))) picked = id;
            if (ImGui::IsItemVisible()) {
                const bool hovered = ImGui::IsItemHovered(), chosen = marked == id;
                const ImVec2 q(p.x + tile, p.y + tile);
                dl->AddRectFilled(p, q, tok(hovered ? mcui::tokens::kColorBgHover : mcui::tokens::kColorBgPrimary), 14 * s);
                if (chosen) dl->AddRect(p, q, tok(mcui::tokens::kColorAccent), 14 * s, ImDrawFlags_None, 2 * s);
                std::string label = std::to_string(id) + "  ×" + std::to_string(order[i].first);
                ImVec2 ts = ImGui::CalcTextSize(label.c_str());
                SDL_Texture* tex = nullptr;
                ImVec2 size;
                const float pad = 8 * s, room = tile - 2 * pad - ts.y - 4 * s;
                if (pieceTexture(id, &tex, &size)) {
                    float k = std::min((tile - 2 * pad) / size.x, room / size.y);
                    ImVec2 a(p.x + (tile - size.x * k) / 2, p.y + pad + (room - size.y * k) / 2);
                    dl->AddImage(ImTextureRef(ImTextureID(tex)), a, ImVec2(a.x + size.x * k, a.y + size.y * k));
                }
                dl->AddText(ImVec2(p.x + (tile - ts.x) / 2, q.y - pad - ts.y), tok(chosen ? mcui::tokens::kColorTextPrimary : mcui::tokens::kColorTextMuted),
                            label.c_str());
            }
            ImGui::PopID();
        }
        return picked;
    }

    bool tileButton(const mcui::Icon& icon, ImU32 color, const char* name, float size) {
        const float s = m_scale;
        ImVec2 p = ImGui::GetCursorScreenPos();
        bool pressed = ImGui::InvisibleButton("##tile", ImVec2(size, size * 0.86f));
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (ImGui::IsItemHovered()) dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size * 0.86f), faded(tok(mcui::tokens::kColorBgHover)), 10 * s);
        const float is = 22 * s;
        mcui::DrawIcon(dl, icon, ImVec2(p.x + (size - is) / 2, p.y + 9 * s), is, faded(color));
        ImGui::PushFont(nullptr, mcui::kTextCaption);
        ImVec2 ts = ImGui::CalcTextSize(name);
        dl->AddText(ImVec2(p.x + (size - ts.x) / 2, p.y + size * 0.86f - ts.y - 7 * s), faded(tok(mcui::tokens::kColorTextSecondary)), name);
        ImGui::PopFont();
        return pressed;
    }

    bool fixInput(const char* id, Fix* v, float width) {
        float f = toF(*v);
        ImGui::PushID(id);
        ImGui::SetNextItemWidth(width);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8 * m_scale, ImGui::GetStyle().FramePadding.y));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(2 * m_scale, 2 * m_scale));
        bool changed = ImGui::InputFloat("##value", &f, 0.25f, 1.0f, "%.2f");
        ImGui::PopStyleVar(2);
        ImGui::PopID();
        if (ImGui::IsItemActivated()) pushUndo(&doc);
        if (changed) *v = toFix(f);
        return changed;
    }
    bool fixField(const char* label, Fix* v) {
        ImGui::TextDisabled("%s", label);
        return fixInput(label, v, -FLT_MIN);
    }
    void fixPair(const char* a, Fix* va, const char* b, Fix* vb) {
        const float gap = ImGui::GetStyle().ItemSpacing.x;
        const float half = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
        const float x0 = ImGui::GetCursorPosX();
        ImGui::TextDisabled("%s", a);
        ImGui::SameLine(x0 + half + gap);
        ImGui::TextDisabled("%s", b);
        fixInput(a, va, half);
        ImGui::SameLine();
        fixInput(b, vb, half);
    }

    void addHereMenu() {
        const float s = m_scale;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8 * s, 8 * s));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4 * s, 4 * s));
        const bool open = ImGui::BeginPopup("##addhere");
        ImGui::PopStyleVar(2);
        if (!open) return;
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        const float x = m_addX, y = m_addY, tile = 70 * s;
        ImGui::PushFont(nullptr, mcui::kTextCaption);
        ImGui::TextDisabled("%s · x %.2f  y %.2f", L("Добавить сюда", "Add here"), x, y);
        ImGui::PopFont();
        const Icons& ic = icons();
        const ImU32 text = tok(mcui::tokens::kColorTextPrimary);
        struct Tile {
            Tool tool;
            const mcui::Icon* icon;
            ImU32 color;
            const char* name;
            bool enabled;
        };
        const Node* dish = nullptr;
        const Tile tiles[] = {{Tool::Block, &ic.block, text, L("Коробка", "Block"), true},
                              {Tool::RampL, &ic.rampL, text, L("Рампа −x", "Ramp −x"), true},
                              {Tool::RampR, &ic.rampR, text, L("Рампа +x", "Ramp +x"), true},
                              {Tool::Bag, &ic.bag, tok(mcui::tokens::kColorAttention), L("Сумка", "Bag"), true},
                              {Tool::Checkpoint, &ic.flag, kAccentLight, L("Чекпоинт", "Checkpoint"), checkpointModel(&dish) != nullptr},
                              {Tool::Zip, &ic.zip, text, L("Трос", "Zip line"), true},
                              {Tool::Swing, &ic.swing, text, L("Турник", "Swing bar"), true},
                              {Tool::Music, &ic.music, text, L("Музыка", "Music"), true},
                              {Tool::Piece, &ic.piece, text, L("Кусок", "Piece"), !m_paletteIds.empty()}};
        int picked = m_demoPickBag ? 3 : -1;
        m_demoPickBag = false;
        for (int i = 0; i < int(sizeof tiles / sizeof tiles[0]); i++) {
            if (i % 4) ImGui::SameLine();
            ImGui::PushID(i);
            ImGui::BeginDisabled(!tiles[i].enabled);
            if (tileButton(*tiles[i].icon, tiles[i].color, tiles[i].name, tile)) picked = i;
            ImGui::EndDisabled();
            if (i == 7 && ImGui::IsItemHovered() && m_paletteIds.count(m_piece)) ImGui::SetTooltip("%s %d", L("кусок", "piece"), m_piece);
            ImGui::PopID();
        }
        if (picked >= 0) {
            const Tool tool = tiles[picked].tool;
            if (tool == Tool::Piece && !m_paletteIds.count(m_piece)) {
                setTool(Tool::Piece);
            } else {
                const Tool was = m_tool;
                m_tool = tool;
                placeTool(x, y);
                m_tool = was;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    std::string coverText(const Shape& s) {
        const float ax = toF(s.box[0]), ay = toF(s.box[1]), bx = toF(s.box[2]), by = toF(s.box[3]);
        std::vector<int> ids;
        for (auto& l : doc.map.layers)
            for (auto& c : l.columns)
                for (auto& sec : c.sections)
                    for (auto& n : sec.nodes) {
                        std::array<float, 6> b;
                        if (!nodeBox(n, &b)) continue;
                        const float x = toF(n.pos[0]), y = toF(n.pos[1]);
                        const float ox = std::min(bx, x + b[3]) - std::max(ax, x + b[0]);
                        const float oy = std::min(by, y + b[4]) - std::max(ay, y + b[1]);
                        if (ox > 0.01f && oy > 0.01f && std::find(ids.begin(), ids.end(), n.id) == ids.end()) ids.push_back(n.id);
                    }
        if (ids.empty()) return L("Куски палитры на неё не заходят.", "No palette piece is over it.");
        std::string list;
        for (size_t i = 0; i < ids.size() && i < 4; i++) list += (i ? ", " : "") + std::to_string(ids[i]);
        if (ids.size() > 4) list += std::string(L(" и ещё ", " and ")) + std::to_string(ids.size() - 4) + L("", " more");
        return std::string(L("Закрыта кусками: ", "Covered by pieces: ")) + list + ".";
    }
    void properties() {
        Map& m = doc.map;
        if (!selValid(m, sel)) return;
        switch (sel.kind) {
        case Kind::Shape: {
            Shape& s = m.shapes[size_t(sel.index)];
            const bool zone = s.layer != 0;
            if (zone) {
                caption(L("Зона: по ней не бегают и в игре её не видно — она меняет, что делает Фейт внутри неё («i» у слоя).",
                          "A zone: you don't run on it and the game doesn't show it — it changes what Faith does inside it (the "
                          "i next to layer)."));
            } else if (port::platform::shapeSurface(s.material)) {
                caption(L("По ней бегают и прыгают. В игре покрыта поверхностью куска: его фасад — по лицевой стороне, крыша — "
                          "по верху.",
                          "You run and jump on it. The game covers it with the piece's surface: its facade on the front, its "
                          "roof on top."));
            } else {
                caption(supportsShown()
                            ? L("По ней бегают и прыгают. В игре видна белой фигурой — так включено в карточке уровня («Видимые "
                                "коробки и рампы»). Чтобы выглядела как город — поверхность куска или «Покрыть куском».",
                                "You run and jump on it. The game shows it as a white shape — set in the level card (Visible "
                                "blocks and ramps). To make it look like the city — a piece surface or Cover with a piece.")
                            : L("По ней бегают и прыгают. В игре её не видно — так выключено в карточке уровня («Видимые коробки "
                                "и рампы»). Видно куски палитры и поверхность куска — её выбор здесь или «Покрыть куском».",
                                "You run and jump on it. The game does not show it — turned off in the level card (Visible blocks "
                                "and ramps). Palette pieces and a piece surface are what you see — choose it here or Cover with a piece."));
            }
            if (!m_pieceBox.empty() && !zone) caption(coverText(s).c_str());
            if (!zone) surfaceRow(s);
            const char* types[] = {L("коробка", "block"), L("рампа к -x", "ramp to -x"), L("рампа к +x", "ramp to +x")};
            int type = std::max(0, std::min(2, int(s.type)));
            ImGui::SetNextItemWidth(176 * m_scale);
            if (ImGui::Combo(L("форма", "shape"), &type, types, 3)) pushUndo(&doc), s.type = int8_t(type);
            const char* layers[] = {L("0 твёрдая", "0 solid"), L("1 бег по стене", "1 wall run"), L("2 балка", "2 balance beam"),
                                    L("3 дальний прыжок", "3 long jump")};
            const char* layerHelp[] = {
                L("По ней бегают, на неё прыгают, в неё упираются. Видна в игре белой, если опоры видны.",
                  "You run and jump on it and bump into it. Shown white in the game when supports are visible."),
                L("Зона, проходимая и невидимая: прыжок на бегу, пока Фейт в ней, — бег боком по стене позади неё (до "
                  "секунды). Ставьте над разрывом перед стеной или вывеской на фоне, на высоте бега; твёрдой стены не нужно.",
                  "A zone, walk-through and invisible: a running jump while Faith is in it becomes a run along the wall behind "
                  "her (up to a second). Put it over a gap in front of a wall or a sign in the background, at running height; "
                  "no solid wall is needed."),
                L("Зона, проходимая и невидимая: на бегу в ней Фейт идёт по балке, балансируя. Ставьте над узкой твёрдой "
                  "формой — балкой, трубой.",
                  "A zone, walk-through and invisible: running in it, Faith walks a balance beam. Put it over a narrow solid "
                  "shape — a beam, a pipe."),
                L("Зона, проходимая и невидимая: прыжок из неё — дальний, быстрее и выше обычного. Ставьте маленькой у края "
                  "перед большим пролётом.",
                  "A zone, walk-through and invisible: a jump from it is a long one, faster and higher. Put a small one at "
                  "the edge before a big gap.")};
            ImGui::SetNextItemWidth(176 * m_scale);
            if (ImGui::Combo(L("слой", "layer"), &s.layer, layers, 4)) pushUndo(&doc);
            infoButton("##info-layer", layerHelp[std::max(0, std::min(3, int(s.layer)))]);
            const char* materials[] = {L("0 бетон", "0 concrete"), L("1 металлический короб", "1 metal duct"),
                                       L("2 деревянные доски", "2 wooden planks"), L("3 металлические мостки", "3 metal gantry"),
                                       L("4 металл крана", "4 crane metal"), L("5 металлические мостки", "5 metal gantry"),
                                       L("6 бетон, видна тёмной", "6 concrete, shown dark"),
                                       L("7 бетон, видна белой", "7 concrete, shown white")};
            int material = std::max(0, std::min(7, port::platform::shapeMaterialIndex(s.material)));
            ImGui::SetNextItemWidth(176 * m_scale);
            if (ImGui::Combo(L("материал", "material"), &material, materials, 8))
                pushUndo(&doc), s.material = port::platform::withSurface(material, port::platform::shapeSurface(s.material));
            const char* materialHelp =
                s.layer == 2 || s.layer == 3 ? L("У этой зоны материал ни на что не влияет: шаги звучат по твёрдой форме под ногами.",
                                                 "The material of this zone changes nothing: footsteps sound by the solid shape underfoot.")
                : s.layer == 1               ? L("Звук шагов при беге по стене в этой зоне.", "The footstep sound of a wall run in this zone.")
                : material >= 6 ? L("Звук шагов — бетон. Форма видна в игре, даже если «Видимые коробки и рампы» выключены.",
                                    "Footsteps — concrete. The shape is shown in the game even with Visible blocks and ramps off.")
                                : L("Звук шагов по ней. На бег и прыжки не влияет.", "The footstep sound on it. Running and jumps don't change.");
            infoButton("##info-material", materialHelp);
            fixPair("min x", &s.box[0], "min y", &s.box[1]);
            fixPair("max x", &s.box[2], "max y", &s.box[3]);
            break;
        }
        case Kind::Node: {
            Node& n = m.layers[size_t(sel.layer)].columns[size_t(sel.column)].sections[size_t(sel.section)].nodes[size_t(sel.index)];
            fixPair("x", &n.pos[0], "y", &n.pos[1]);
            depthRow(n);
            fixField("z", &n.pos[2]);
            mirrorRow(n);
            break;
        }
        case Kind::Object: {
            Object& o = m.objects[size_t(sel.index)];
            if (o.type == kBag && !doc.bags)
                caption(L("В гонке сумок нет — они только в режиме «Сбор сумок».", "Races have no bags — only the Bag collection mode does."));
            if (o.type == kSwing) {
                caption(L("Точка хвата: Фейт допрыгнула — повисает на 1,6 ниже и раскачивается (наклон — сильнее); прыжок или "
                          "свайп вперёд, когда её качнуло вперёд, — соскок вперёд и вверх. Её в игре не видно — перекладину "
                          "рисует кусок палитры (труба, балка, леса) через эту точку.",
                          "A grab point: when Faith jumps into it, she hangs 1.6 below and swings (tilt swings harder); a jump or a "
                          "forward swipe as she swings forward launches her forward and up. The game doesn't show it — put a "
                          "palette piece (a pipe, a beam, scaffolding) through this point as the bar."));
                infoButton("##info-swing", L("Высота: с ровного места с разбега Фейт достаёт турник на 3 над полом; в главах "
                                             "турники висят и на 6–8 над провалом — к ним прыгают с края выше. Двумя турниками "
                                             "через 6 перелетают провал.",
                                             "Height: from a running jump on flat ground Faith reaches a bar 3 above the floor; in "
                                             "the chapters bars also hang 6–8 above a gap — you jump to them from a higher edge. "
                                             "Two bars 6 apart carry you over a gap."));
            }
            fixPair("x", &o.pos[0], "y", &o.pos[1]);
            break;
        }
        case Kind::Start: {
            Start& s = m.starts[size_t(sel.index)];
            fixPair("x", &s.pos[0], "y", &s.pos[1]);
            bool right = s.facing > 0;
            if (ImGui::Checkbox(L("лицом вправо", "facing right"), &right)) pushUndo(&doc), s.facing = int8_t(right ? 1 : -1);
            break;
        }
        case Kind::Finish: {
            auto& f = m.finishes[size_t(sel.index)];
            fixPair("x", &f[0], "y", &f[1]);
            fixPair(L("ширина", "width"), &f[2], L("высота", "height"), &f[3]);
            break;
        }
        case Kind::Checkpoint: {
            Checkpoint& c = m.checkpoints[size_t(sel.index)];
            checkpointDish(sel.index);
            fixPair("min x", &c.box[0], "min y", &c.box[1]);
            fixPair("max x", &c.box[2], "max y", &c.box[3]);
            break;
        }
        case Kind::Sound: {
            auto& list = sel.list == 0 ? m.ambientSounds : sel.list == 1 ? m.oneShots : m.musicCues;
            Sound& s = list[size_t(sel.index)];
            caption(sel.list == 2 ? L("Невидимая метка: добежали — играет эта музыка.", "An invisible marker: reach it and this music plays.")
                                  : L("Невидимая метка звука.", "An invisible sound marker."));
            fieldLabel(sel.list == 2 ? L("Трек", "Track") : L("Звук", "Sound"));
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##sound", soundName(s.res).c_str())) {
                for (int i : soundChoices(sel.list == 2))
                    if (ImGui::Selectable(soundName(i).c_str(), i == s.res)) pushUndo(&doc), s.res = i;
                ImGui::EndCombo();
            }
            fixPair("x", &s.pos[0], "y", &s.pos[1]);
            break;
        }
        case Kind::ZipStart:
        case Kind::ZipEnd: {
            auto& z = m.zipLines[size_t(sel.index)];
            fixPair("x1", &z[0], "y1", &z[1]);
            fixPair("x2", &z[2], "y2", &z[3]);
            fixField(L("отступ конца", "end offset"), &z[4]);
            break;
        }
        default: break;
        }
    }

    void canvas() {
        Map& m = doc.map;
        keepGroup();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        view.origin = ImGui::GetCursorScreenPos();
        view.size = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("##canvas", view.size,
                               ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        bool hovered = ImGui::IsItemHovered();
        ImGuiIO& io = ImGui::GetIO();
        dl->PushClipRect(view.origin, ImVec2(view.origin.x + view.size.x, view.origin.y + view.size.y), true);
        dl->AddRectFilled(view.origin, ImVec2(view.origin.x + view.size.x, view.origin.y + view.size.y), IM_COL32(28, 30, 36, 255));
        float step = 8;
        float x0, y0, x1, y1;
        view.toWorld(view.origin, &x0, &y1);
        view.toWorld(ImVec2(view.origin.x + view.size.x, view.origin.y + view.size.y), &x1, &y0);
        if (view.zoom * step > 6)
            for (float x = floorf(x0 / step) * step; x <= x1; x += step)
                dl->AddLine(view.toScreen(x, y0), view.toScreen(x, y1), IM_COL32(255, 255, 255, 14));
        if (view.zoom * step > 6)
            for (float y = floorf(y0 / step) * step; y <= y1; y += step)
                dl->AddLine(view.toScreen(x0, y), view.toScreen(x1, y), IM_COL32(255, 255, 255, 14));
        dl->AddLine(view.toScreen(toF(m.minX), y0), view.toScreen(toF(m.minX), y1), IM_COL32(200, 60, 60, 120));
        dl->AddLine(view.toScreen(toF(m.maxX), y0), view.toScreen(toF(m.maxX), y1), IM_COL32(200, 60, 60, 120));

        auto selected = [&](Kind k, int i, int list = 0) {
            Sel q;
            q.kind = k, q.index = i, q.list = list;
            return inSelection(q);
        };
        if (showGraphics) drawPieceViews(dl);
        if (showGraphics)
            for (size_t l = 0; l < m.layers.size(); l++)
                for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                    for (size_t s = 0; s < m.layers[l].columns[c].sections.size(); s++) {
                        const Section& sec = m.layers[l].columns[c].sections[s];
                        dl->AddRect(view.toScreen(toF(sec.bounds[0]), toF(sec.bounds[4])),
                                    view.toScreen(toF(sec.bounds[3]), toF(sec.bounds[1])), IM_COL32(90, 130, 200, 40));
                        for (size_t n = 0; n < sec.nodes.size(); n++) {
                            const Node& node = sec.nodes[n];
                            ImVec2 p = view.toScreen(toF(node.pos[0]), toF(node.pos[1]));
                            Sel me;
                            me.kind = Kind::Node, me.layer = int(l), me.column = int(c), me.section = int(s), me.index = int(n);
                            const bool on = inSelection(me);
                            std::array<float, 6> box;
                            if (nodeBox(node, &box)) {
                                const float x = toF(node.pos[0]), y = toF(node.pos[1]);
                                ImVec2 a = view.toScreen(x + box[0], y + box[4]), b = view.toScreen(x + box[3], y + box[1]);
                                dl->AddRectFilled(a, b, on ? IM_COL32(90, 150, 230, 60) : IM_COL32(90, 150, 230, 14));
                                dl->AddRect(a, b, on ? IM_COL32(255, 255, 255, 230) : IM_COL32(90, 150, 230, 70), 0.0f, ImDrawFlags_None,
                                            on ? 2.0f : 1.0f);
                            }
                            float r = 3 * m_scale;
                            dl->AddRectFilled(ImVec2(p.x - r, p.y - r), ImVec2(p.x + r, p.y + r),
                                              on ? IM_COL32(255, 255, 255, 255)
                                                 : node.visionFlag == 1 ? IM_COL32(230, 60, 50, 200) : IM_COL32(90, 150, 230, 200));
                            if (view.zoom > 6 * m_scale)
                                dl->AddText(ImVec2(p.x + r + 1, p.y - r - 1), IM_COL32(120, 160, 230, 160), std::to_string(node.id).c_str());
                        }
                    }
        if (showShapes)
            for (size_t i = 0; i < m.shapes.size(); i++) {
                const Shape& s = m.shapes[i];
                float ax = toF(s.box[0]), ay = toF(s.box[1]), bx = toF(s.box[2]), by = toF(s.box[3]);
                ImU32 fill = layerColor(s.layer, selected(Kind::Shape, int(i)) ? 170 : 70);
                ImU32 edge = layerColor(s.layer, 230);
                if (showGraphics && drawSurface(dl, s)) fill = selected(Kind::Shape, int(i)) ? layerColor(s.layer, 60) : 0;
                if (s.type == 1) {
                    dl->AddTriangleFilled(view.toScreen(ax, ay), view.toScreen(bx, ay), view.toScreen(ax, by), fill);
                    dl->AddTriangle(view.toScreen(ax, ay), view.toScreen(bx, ay), view.toScreen(ax, by), edge);
                } else if (s.type == 2) {
                    dl->AddTriangleFilled(view.toScreen(ax, ay), view.toScreen(bx, ay), view.toScreen(bx, by), fill);
                    dl->AddTriangle(view.toScreen(ax, ay), view.toScreen(bx, ay), view.toScreen(bx, by), edge);
                } else {
                    dl->AddRectFilled(view.toScreen(ax, by), view.toScreen(bx, ay), fill);
                    dl->AddRect(view.toScreen(ax, by), view.toScreen(bx, ay), edge);
                }
            }
        if (showShapes && showGraphics)
            for (auto& l : m.layers)
                for (auto& c : l.columns)
                    for (auto& sec : c.sections)
                        for (auto& node : sec.nodes) {
                            std::array<float, 6> box;
                            if (!nodeBox(node, &box)) continue;
                            const float x = toF(node.pos[0]), y = toF(node.pos[1]), in = 2.0f;
                            ImVec2 a = view.toScreen(x + box[0], y + box[4]), b = view.toScreen(x + box[3], y + box[1]);
                            if (b.x - a.x > 3 * in && b.y - a.y > 3 * in)
                                dl->AddRect(ImVec2(a.x + in, a.y + in), ImVec2(b.x - in, b.y - in), IM_COL32(90, 150, 230, 150));
                        }
        if (showObjects) {
            float r = 6 * m_scale;
            auto dot = [&](float x, float y, ImU32 col, const char* text, bool on) {
                ImVec2 p = view.toScreen(x, y);
                dl->AddCircleFilled(p, r, col);
                if (on) dl->AddCircle(p, r + 3, IM_COL32(255, 255, 255, 255), 0, 2);
                dl->AddText(ImVec2(p.x + r + 2, p.y - r), col, text);
            };
            for (size_t i = 0; i < m.checkpoints.size(); i++) {
                const Checkpoint& c = m.checkpoints[i];
                dl->AddRect(view.toScreen(toF(c.box[0]), toF(c.box[3])), view.toScreen(toF(c.box[2]), toF(c.box[1])),
                            selected(Kind::Checkpoint, int(i)) ? IM_COL32(255, 255, 255, 255) : IM_COL32(80, 170, 255, 220), 0.0f, ImDrawFlags_None, 2.0f);
                dl->AddText(view.toScreen(toF(c.box[0]), toF(c.box[3])), IM_COL32(80, 170, 255, 220), L("чекпоинт", "checkpoint"));
            }
            for (size_t i = 0; i < m.finishes.size(); i++) {
                auto& f = m.finishes[i];
                ImVec2 a = view.toScreen(toF(f[0]), toF(f[1]) + toF(f[3])), b = view.toScreen(toF(f[0]) + toF(f[2]), toF(f[1]));
                dl->AddRectFilled(a, b, IM_COL32(240, 240, 240, 60));
                dl->AddRect(a, b, selected(Kind::Finish, int(i)) ? IM_COL32(255, 255, 255, 255) : IM_COL32(240, 240, 240, 200), 0.0f, ImDrawFlags_None, 2.0f);
                dl->AddText(a, IM_COL32(240, 240, 240, 230), L("финиш", "finish"));
            }
            for (size_t i = 0; i < m.zipLines.size(); i++) {
                auto& z = m.zipLines[i];
                dl->AddLine(view.toScreen(toF(z[0]), toF(z[1])), view.toScreen(toF(z[2]), toF(z[3])), IM_COL32(230, 230, 120, 220), 2);
                dot(toF(z[0]), toF(z[1]), IM_COL32(230, 230, 120, 255), L("трос", "zip"), selected(Kind::ZipStart, int(i)));
                dot(toF(z[2]), toF(z[3]), IM_COL32(230, 230, 120, 255), "", selected(Kind::ZipEnd, int(i)));
            }
            for (size_t i = 0; i < m.objects.size(); i++) {
                const Object& o = m.objects[i];
                if (o.type == kSwing) {
                    const float x = toF(o.pos[0]), y = toF(o.pos[1]);
                    dl->AddLine(view.toScreen(x - kSwingHalf, y), view.toScreen(x + kSwingHalf, y), IM_COL32(120, 220, 200, 230), 3 * m_scale);
                    dl->AddLine(view.toScreen(x, y), view.toScreen(x, y - kSwingHang), IM_COL32(120, 220, 200, 110), 1.5f * m_scale);
                }
                dot(toF(o.pos[0]), toF(o.pos[1]),
                    o.type == kBag ? IM_COL32(250, 200, 40, 255) : o.type == kSwing ? IM_COL32(120, 220, 200, 255) : IM_COL32(180, 180, 190, 255),
                    objectName(o.type), selected(Kind::Object, int(i)));
            }
            const std::vector<Sound>* lists[] = {&m.ambientSounds, &m.oneShots, &m.musicCues};
            for (int l = 0; l < 3; l++)
                for (size_t i = 0; i < lists[l]->size(); i++) {
                    const Sound& s = (*lists[l])[i];
                    const std::string name = l == 2 ? soundName(s.res) : std::string();
                    dot(toF(s.pos[0]), toF(s.pos[1]), IM_COL32(200, 120, 230, 255), name.c_str(), selected(Kind::Sound, int(i), l));
                }
            for (size_t i = 0; i < m.starts.size(); i++) {
                const Start& s = m.starts[i];
                dot(toF(s.pos[0]), toF(s.pos[1]), IM_COL32(80, 220, 110, 255), L("старт", "start"), selected(Kind::Start, int(i)));
                ImVec2 p = view.toScreen(toF(s.pos[0]), toF(s.pos[1]));
                dl->AddLine(p, ImVec2(p.x + s.facing * 3 * r, p.y), IM_COL32(80, 220, 110, 255), 2);
            }
        }
        if (m_hasArea) {
            float a[4];
            area(a);
            dl->AddRectFilled(view.toScreen(a[0], a[3]), view.toScreen(a[2], a[1]), IM_COL32(250, 210, 60, 30));
            dl->AddRect(view.toScreen(a[0], a[3]), view.toScreen(a[2], a[1]), IM_COL32(250, 210, 60, 220), 0.0f, ImDrawFlags_None, 2.0f);
        }
        dl->PopClipRect();

        float mx, my;
        view.toWorld(io.MousePos, &mx, &my);
        if (hovered && (io.MouseWheel != 0 || io.MouseWheelH != 0)) {
            if (io.KeyCtrl) {
                float k = powf(1.2f, io.MouseWheel);
                view.zoom = std::max(0.2f, std::min(200.0f, view.zoom * k));
                float nx, ny;
                view.toWorld(io.MousePos, &nx, &ny);
                view.cx += mx - nx, view.cy += my - ny;
            } else {
                float wx = io.MouseWheelH, wy = io.MouseWheel;
                if (io.KeyShift && wx == 0) wx = wy, wy = 0;
                const float step = 48 * m_scale / view.zoom;
                view.cx -= wx * step;
                view.cy += wy * step;
            }
        }
        const bool spaceHand = ImGui::IsKeyDown(ImGuiKey_Space) && !io.WantTextInput;
        const bool hand = m_tool == Tool::Hand || spaceHand;
        if (ImGui::IsItemActive() && (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0) ||
                                      (hand && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0)))) {
            view.cx -= io.MouseDelta.x / view.zoom;
            view.cy += io.MouseDelta.y / view.zoom;
        }
        if (hovered && hand) ImGui::SetMouseCursor(ImGui::IsItemActive() ? ImGuiMouseCursor_ResizeAll : ImGuiMouseCursor_Hand);
        if (pinch()) return;
        auto snap = [](float v) { return roundf(v / kSnap) * kSnap; };
        const bool placing = !hand && m_tool != Tool::Select && m_tool != Tool::Area;
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && hand) {
        } else if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && placing && !io.KeyShift) {
            placeTool(snap(mx), snap(my));
        } else if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && (io.KeyShift || m_tool == Tool::Area)) {
            m_areaDrag = m_hasArea = true;
            m_area[0] = m_area[2] = snap(mx), m_area[1] = m_area[3] = snap(my);
        } else if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && io.KeyCtrl) {
            const Sel under = pick(io.MousePos);
            toggleSelected(under);
            m_groupClick = Sel();
            m_dragging = inSelection(under);
            m_dragUndo = false;
            m_dragX = mx, m_dragY = my;
        } else if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && (m_resize = edgesAt(io.MousePos)) != 0) {
            m_dragUndo = false;
        } else if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            const std::vector<Sel> under = picks(io.MousePos);
            m_groupClick = m_cycleTo = Sel();
            auto at = [&](const Sel& s) { return int(std::find_if(under.begin(), under.end(), [&](const Sel& u) { return sameSel(u, s); }) - under.begin()); };
            const Sel* member = nullptr;
            for (const Sel& u : under)
                if (!member && !m_group.empty() && inSelection(u)) member = &u;
            if (member) {
                m_groupClick = *member;
            } else if (selValid(m, sel) && at(sel) < int(under.size())) {
                if (under.size() > 1) m_cycleTo = under[size_t(at(sel) + 1) % under.size()];
            } else {
                sel = under.empty() ? Sel() : under.front();
                m_group.clear();
                m_groupFor = m_stackSel = sel;
                m_stackCount = int(under.size()), m_stackPos = 0;
            }
            m_dragging = sel.kind != Kind::None;
            m_dragUndo = false;
            m_dragX = mx, m_dragY = my;
        }
        if (hovered && !hand && !placing && !m_dragging) {
            int e = m_resize ? m_resize : edgesAt(io.MousePos);
            if (e) ImGui::SetMouseCursor(!(e & (EdgeTop | EdgeBottom))   ? ImGuiMouseCursor_ResizeEW
                                         : !(e & (EdgeLeft | EdgeRight)) ? ImGuiMouseCursor_ResizeNS
                                         : ((e & EdgeLeft) != 0) == ((e & EdgeTop) != 0) ? ImGuiMouseCursor_ResizeNWSE
                                                                                         : ImGuiMouseCursor_ResizeNESW);
        }
        if (m_resize && ImGui::IsMouseDown(ImGuiMouseButton_Left) && selValid(m, sel)) {
            float b[4], nb[4];
            selBox(m, sel, b);
            std::copy(b, b + 4, nb);
            if (m_resize & EdgeLeft) nb[0] = std::min(snap(mx), b[2] - kSnap);
            if (m_resize & EdgeRight) nb[2] = std::max(snap(mx), b[0] + kSnap);
            if (m_resize & EdgeBottom) nb[1] = std::min(snap(my), b[3] - kSnap);
            if (m_resize & EdgeTop) nb[3] = std::max(snap(my), b[1] + kSnap);
            if (!std::equal(b, b + 4, nb)) {
                if (!m_dragUndo) pushUndo(&doc), m_dragUndo = true;
                setSelBox(m, sel, nb);
            }
        } else {
            m_resize = 0;
        }
        if (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
            io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] <= io.MouseDragThreshold * io.MouseDragThreshold) {
            m_addX = snap(mx), m_addY = snap(my);
            ImGui::OpenPopup("##addhere");
        }
        const float still = 12 * m_scale;
        if (m_touchSeen && hovered && !hand && !placing && m_tool != Tool::Area && m_fingers.size() == 1 && !m_longPress &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left) && io.MouseDownDuration[ImGuiMouseButton_Left] >= kLongPressSec &&
            io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] <= still * still) {
            m_longPress = true;
            m_dragging = false;
            m_resize = 0;
            m_addX = snap(mx), m_addY = snap(my);
            ImGui::OpenPopup("##addhere");
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_longPress = false;
        if (m_areaDrag) {
            m_area[2] = snap(mx), m_area[3] = snap(my);
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) m_areaDrag = false;
            if (!m_areaDrag && m_area[0] == m_area[2] && m_area[1] == m_area[3]) m_hasArea = false;
        }
        if (m_dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            float dx = roundf((mx - m_dragX) / kSnap) * kSnap, dy = roundf((my - m_dragY) / kSnap) * kSnap;
            if ((dx != 0 || dy != 0) && selValid(m, sel)) {
                if (!m_dragUndo) pushUndo(&doc), m_dragUndo = true;
                for (const Sel& one : selection()) {
                    const bool dish = one.kind != Kind::Checkpoint || !inSelection(refSel(m.checkpoints[size_t(one.index)].ref));
                    moveSel(m, one, dx, dy, dish);
                }
                m_dragX += dx, m_dragY += dy;
            }
        } else {
            if (m_dragging && !m_dragUndo && m_groupClick.kind != Kind::None && selValid(m, m_groupClick))
                sel = m_groupClick, m_group.clear(), m_groupFor = sel;
            if (m_dragging && !m_dragUndo && m_cycleTo.kind != Kind::None && selValid(m, m_cycleTo)) {
                const std::vector<Sel> under = picks(io.MousePos);
                sel = m_groupFor = m_stackSel = m_cycleTo;
                m_stackCount = int(under.size());
                m_stackPos = int(std::find_if(under.begin(), under.end(), [&](const Sel& u) { return sameSel(u, sel); }) - under.begin());
            }
            m_groupClick = m_cycleTo = Sel();
            m_dragging = false;
        }
        if (hovered && !hand && !placing && !m_dragging && !m_areaDrag && showGraphics && !ImGui::IsPopupOpen("##addhere")) {
            Sel under = pick(io.MousePos);
            if (under.kind == Kind::Node && !(under.layer == sel.layer && under.column == sel.column && under.section == sel.section &&
                                               under.index == sel.index && sel.kind == Kind::Node)) {
                const Node& n = m.layers[size_t(under.layer)].columns[size_t(under.column)].sections[size_t(under.section)].nodes[size_t(under.index)];
                ImGui::BeginTooltip();
                ImGui::Text("%s %d", L("Кусок", "Piece"), n.id);
                pieceImage(n.id, 240 * m_scale, 160 * m_scale);
                ImGui::EndTooltip();
            }
        }
        ImDrawList* top = ImGui::GetWindowDrawList();
        top->PushClipRect(view.origin, ImVec2(view.origin.x + view.size.x, view.origin.y + view.size.y), true);
        if (placing && hovered && !ImGui::IsPopupOpen("##addhere")) ghost(top, snap(mx), snap(my));
        handles(top);
        const ImU32 muted = tok(mcui::tokens::kColorTextMuted);
        const float edge = 16 * m_scale;
        if (hovered) {
            char xy[64];
            snprintf(xy, sizeof xy, "x %.2f · y %.2f", snap(mx), snap(my));
            ImVec2 ts = ImGui::CalcTextSize(xy);
            top->AddText(ImVec2(view.origin.x + view.size.x - edge - ts.x, view.origin.y + view.size.y - edge - ts.y), muted, xy);
        }
        const float room = (view.size.x - m_viewBarW) / 2 - 2 * edge;
        const std::string placeHint = std::string(toolName(m_tool)) + L(": коснитесь поля — поставить · стрелка на рейке — выбор",
                                                                         ": tap the field — place · arrow on the rail — select");
        const char* hint = m_touchSeen && hand ? L("Рука: палец — двигать вид · два пальца — вид и масштаб · стрелка на рейке — выбор",
                                                   "Hand: one finger — pan · two fingers — pan and zoom · arrow on the rail — select")
                           : m_touchSeen && placing ? placeHint.c_str()
                           : m_touchSeen && m_tool == Tool::Area
                               ? L("Область: тяните пальцем рамку — над ней «Как в игре», заполнить и удалить тайлы · два пальца — вид",
                                   "Area: drag a frame with a finger — above it: As in the game, fill and remove tiles · two fingers — view")
                           : m_touchSeen ? L("Палец — выбрать и тащить · ещё касание — то, что за ним · подержать — добавить сюда · "
                                             "два пальца — вид и масштаб · рамка на рейке (третья) — выделить область",
                                             "One finger — select and drag · tap again — the one behind · hold — add here · two "
                                             "fingers — pan and zoom · the frame on the rail (third) — select an area")
                           : hand      ?L("Тяните — двигать вид · Ctrl + колесо, + и − — масштаб · 0 — вписать · V — выбор",
                                           "Drag — pan · Ctrl + wheel, + and − — zoom · 0 — fit · V — select")
                           : placing   ? L("Щелчок — поставить · Esc — выбор · щелчок ПКМ — меню «Добавить сюда»",
                                           "Click — place · Esc — select · RMB click — Add here menu")
                           : m_tool == Tool::Area ? L("Тяните — область · Esc — выбор", "Drag — area · Esc — select")
                                                  : L("ЛКМ — выбрать и тащить · ещё щелчок — то, что за ним · Ctrl+щелчок — добавить к "
                                                      "выбранным · за край или угол — растянуть · Shift — область · колесо, два пальца, "
                                                      "пробел + тянуть — двигать вид · Ctrl + колесо — масштаб · щелчок ПКМ — добавить сюда · "
                                                      "Del — удалить",
                                                      "LMB — select and drag · click again — the one behind · Ctrl+click — add to the "
                                                      "selection · edge or corner — resize · Shift — area · wheel, two fingers, Space + drag — "
                                                      "pan · Ctrl + wheel — zoom · RMB click — add here · Del — delete");
        if (room > 120 * m_scale) {
            ImFont* font = ImGui::GetFont();
            const float size = ImGui::GetFontSize() * mcui::kTextCaption / mcui::kTextBody;
            ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, room, hint);
            top->AddText(font, size, ImVec2(view.origin.x + edge, view.origin.y + view.size.y - edge - ts.y), muted, hint, nullptr, room);
        }
        top->PopClipRect();
    }

    bool pinch() {
        if (m_fingers.empty()) m_pinch = m_pinchHold = false;
        if (m_fingers.size() < 2) {
            m_pinch = false;
            return m_pinchHold;
        }
        auto it = m_fingers.begin();
        ImVec2 a = it->second, b = (++it)->second;
        ImVec2 mid((a.x + b.x) / 2, (a.y + b.y) / 2);
        float dist = std::max(1.0f, sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)));
        if (!m_pinch) {
            bool inside = mid.x >= view.origin.x && mid.y >= view.origin.y && mid.x < view.origin.x + view.size.x &&
                          mid.y < view.origin.y + view.size.y;
            if (!inside) return m_pinchHold;
            if ((m_dragging || m_resize) && m_dragUndo) undo();
            m_dragging = false;
            m_resize = 0;
            if (m_areaDrag) m_areaDrag = m_hasArea = false;
            m_pinch = m_pinchHold = true;
            view.toWorld(mid, &m_pinchX, &m_pinchY);
            m_pinchZoom = view.zoom;
            m_pinchDist = dist;
        }
        view.zoom = std::max(0.2f, std::min(200.0f, m_pinchZoom * dist / m_pinchDist));
        view.cx = m_pinchX - (mid.x - view.origin.x - view.size.x / 2) / view.zoom;
        view.cy = m_pinchY + (mid.y - view.origin.y - view.size.y / 2) / view.zoom;
        return true;
    }

    Sel pick(ImVec2 p) {
        const std::vector<Sel> all = picks(p);
        return all.empty() ? Sel() : all.front();
    }
    std::vector<Sel> picks(ImVec2 p) {
        Map& m = doc.map;
        float r = 8 * m_scale;
        auto hit = [&](float x, float y) {
            ImVec2 q = view.toScreen(x, y);
            return fabsf(q.x - p.x) <= r && fabsf(q.y - p.y) <= r;
        };
        float wx, wy;
        view.toWorld(p, &wx, &wy);
        auto inside = [&](float ax, float ay, float bx, float by) { return wx >= ax && wx <= bx && wy >= ay && wy <= by; };
        std::vector<Sel> all;
        auto add = [&](Kind k, int i, int list = 0) {
            Sel s;
            s.kind = k, s.index = i, s.list = list;
            all.push_back(s);
        };
        auto addNode = [&](size_t l, size_t c, size_t k, size_t n) {
            Sel s;
            s.kind = Kind::Node, s.index = int(n), s.layer = int(l), s.column = int(c), s.section = int(k);
            for (const Sel& o : all)
                if (sameSel(o, s)) return;
            all.push_back(s);
        };
        if (showObjects) {
            for (size_t i = 0; i < m.starts.size(); i++)
                if (hit(toF(m.starts[i].pos[0]), toF(m.starts[i].pos[1]))) add(Kind::Start, int(i));
            for (size_t i = 0; i < m.objects.size(); i++)
                if (hit(toF(m.objects[i].pos[0]), toF(m.objects[i].pos[1]))) add(Kind::Object, int(i));
            const std::vector<Sound>* lists[] = {&m.ambientSounds, &m.oneShots, &m.musicCues};
            for (int l = 0; l < 3; l++)
                for (size_t i = 0; i < lists[l]->size(); i++)
                    if (hit(toF((*lists[l])[i].pos[0]), toF((*lists[l])[i].pos[1]))) add(Kind::Sound, int(i), l);
            for (size_t i = 0; i < m.zipLines.size(); i++) {
                if (hit(toF(m.zipLines[i][0]), toF(m.zipLines[i][1]))) add(Kind::ZipStart, int(i));
                if (hit(toF(m.zipLines[i][2]), toF(m.zipLines[i][3]))) add(Kind::ZipEnd, int(i));
            }
            for (size_t i = 0; i < m.finishes.size(); i++) {
                auto& f = m.finishes[i];
                if (inside(toF(f[0]), toF(f[1]), toF(f[0]) + toF(f[2]), toF(f[1]) + toF(f[3]))) add(Kind::Finish, int(i));
            }
            for (size_t i = 0; i < m.checkpoints.size(); i++) {
                auto& c = m.checkpoints[i].box;
                if (inside(toF(c[0]), toF(c[1]), toF(c[2]), toF(c[3]))) add(Kind::Checkpoint, int(i));
            }
        }
        if (showGraphics)
            for (size_t l = 0; l < m.layers.size(); l++)
                for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                    for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++) {
                        auto& nodes = m.layers[l].columns[c].sections[k].nodes;
                        for (size_t n = 0; n < nodes.size(); n++)
                            if (hit(toF(nodes[n].pos[0]), toF(nodes[n].pos[1]))) addNode(l, c, k, n);
                    }
        if (showShapes)
            for (size_t i = m.shapes.size(); i-- > 0;) {
                auto& b = m.shapes[i].box;
                if (inside(toF(b[0]), toF(b[1]), toF(b[2]), toF(b[3]))) add(Kind::Shape, int(i));
            }
        if (showGraphics) {
            struct Boxed {
                float area;
                size_t l, c, k, n;
            };
            std::vector<Boxed> boxed;
            for (size_t l = 0; l < m.layers.size(); l++)
                for (size_t c = 0; c < m.layers[l].columns.size(); c++)
                    for (size_t k = 0; k < m.layers[l].columns[c].sections.size(); k++) {
                        auto& nodes = m.layers[l].columns[c].sections[k].nodes;
                        for (size_t n = 0; n < nodes.size(); n++) {
                            std::array<float, 6> b;
                            if (!nodeBox(nodes[n], &b)) continue;
                            const float nx = toF(nodes[n].pos[0]), ny = toF(nodes[n].pos[1]);
                            if (wx < nx + b[0] || wx > nx + b[3] || wy < ny + b[1] || wy > ny + b[4]) continue;
                            boxed.push_back({(b[3] - b[0]) * (b[4] - b[1]), l, c, k, n});
                        }
                    }
            std::stable_sort(boxed.begin(), boxed.end(), [](const Boxed& a, const Boxed& b) { return a.area < b.area; });
            for (const Boxed& b : boxed) addNode(b.l, b.c, b.k, b.n);
        }
        return all;
    }
};

bool childRunning(long long pid) {
#ifdef _WIN32
    (void)pid;
    return false;
#else
    int status;
    return pid > 0 && waitpid(pid_t(pid), &status, WNOHANG) == 0;
#endif
}

}

int run(const std::string& id, const char* argv0) {
    s_ru = port::platform::launcherRussian();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "medecomp: редактор: SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    const int kWidth = 1280, kHeight = 800;
#ifdef __ANDROID__
    const Uint32 kFlags = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI;
#elif defined(__EMSCRIPTEN__)
    const Uint32 kFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#else
    const Uint32 kFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
#endif
    int createW = kWidth, createH = kHeight;
#ifdef __EMSCRIPTEN__
    {
        double cw = 0, ch = 0;
        emscripten_get_element_css_size("#canvas", &cw, &ch);
        if (cw > 0 && ch > 0) createW = int(cw), createH = int(ch);
    }
#endif
    SDL_Window* window = SDL_CreateWindow(L(MEDECOMP_PORT_NAME " — редактор уровней", MEDECOMP_PORT_NAME " — level editor"),
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, createW, createH, kFlags);
    if (!window) {
        fprintf(stderr, "medecomp: редактор: окно: %s\n", SDL_GetError());
        return 1;
    }
    int pixelW = 0, pixelH = 0;
    SDL_GetWindowSizeInPixels(window, &pixelW, &pixelH);
    float scale = port::platform::displayScale(window, pixelW);
#ifdef __ANDROID__
    scale = 0.8f * std::max(1.0f, std::min(scale,
        std::min(std::max(pixelW, pixelH) / float(kWidth), std::min(pixelW, pixelH) / float(kHeight))));
#elif defined(__EMSCRIPTEN__)
    scale = webEditorScale(float(createW), float(createH));
#else
    SDL_SetWindowSize(window, int(kWidth * scale), int(kHeight * scale));
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "medecomp: редактор: SDL_CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }
    SDL_ShowWindow(window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    if (!port::platform::launcherFonts()) {
        io.Fonts->AddFontDefault();
        s_ru = false;
    }
    mcui::ApplyStyle(scale);
    ImGui::GetStyle().WindowPadding = ImVec2(16 * scale, 16 * scale);
    ImGui::GetStyle().ItemSpacing = ImVec2(8 * scale, 8 * scale);
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    Editor editor(argv0, scale, renderer);
    if (!id.empty()) {
        std::string error;
        if (openLevel(id, &editor.doc, &error)) editor.fit(), editor.loadPalette();
        else fprintf(stderr, "medecomp: редактор: %s\n", error.c_str());
#ifdef MEDECOMP_ONE_PROCESS
        editor.resume();
#endif
    }
    const char* shot = getenv("MEDECOMP_EDITOR_SHOT");
    const char* demo = getenv("MEDECOMP_EDITOR_DEMO");
    const char* demoL8 = getenv("MEDECOMP_EDITOR_DEMO_L8");
    int l8 = 0;
    const char* demoL17 = getenv("MEDECOMP_EDITOR_DEMO_L17");
    int l17 = 0, l17Frame = 0;
    const char* demoL20 = getenv("MEDECOMP_EDITOR_DEMO_L20");
    int l20 = 0;
    const char* demoL19 = getenv("MEDECOMP_EDITOR_DEMO_L19");
    const char* demoL33 = getenv("MEDECOMP_EDITOR_DEMO_L33");
    int l33 = 0, l33Frame = 0;
    ImVec2 l33Mouse;
    const char* demoL29 = getenv("MEDECOMP_EDITOR_DEMO_L29");
    int l29 = 0;
    const char* demoL41 = getenv("MEDECOMP_EDITOR_DEMO_L41");
    int l41 = 0;
    const char* demoL44 = getenv("MEDECOMP_EDITOR_DEMO_L44");
    int l44 = 0;
    const char* demoL45 = getenv("MEDECOMP_EDITOR_DEMO_L45");
    int l45 = 0;
    const char* demoL47 = getenv("MEDECOMP_EDITOR_DEMO_L47");
    int l47 = 0;
    const char* demoL48 = getenv("MEDECOMP_EDITOR_DEMO_L48");
    int l48 = 0;
    const char* demoL52 = getenv("MEDECOMP_EDITOR_DEMO_L52");
    int l52 = 0;
    const char* demoL49 = getenv("MEDECOMP_EDITOR_DEMO_L49");
    int l49 = 0;
    const char* demoL64 = getenv("MEDECOMP_EDITOR_DEMO_L64");
    int l64 = 0;
    const char* demoL63 = getenv("MEDECOMP_EDITOR_DEMO_L63");
    int l63 = 0;
    const char* demoL62 = getenv("MEDECOMP_EDITOR_DEMO_L62");
    int l62 = 0;
    const char* demoL59 = getenv("MEDECOMP_EDITOR_DEMO_L59");
    int l59 = 0, l59Frame = 0;
    const char* demoL58 = getenv("MEDECOMP_EDITOR_DEMO_L58");
    int l58 = 0, l58Frame = 0;
    ImVec2 l58Mouse;
    const char* demoL46 = getenv("MEDECOMP_EDITOR_DEMO_L46");
    int l46 = 0;
    ImVec2 l46Mouse;
    int l19 = 0, l19Frame = 0;
    ImVec2 l19Center, l19Mouse;
    int frames = 0, code = 0;
    bool quit = false, demoStarted = false;
#ifdef __EMSCRIPTEN__
    Uint32 lastFingerAt = 0;
#endif
    while (!quit) {
        SDL_Event e;
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);
        double canvasW = 0, canvasH = 0;
        emscripten_get_element_css_size("#canvas", &canvasW, &canvasH);
        int windowW = 0, windowH = 0;
        SDL_GetWindowSize(window, &windowW, &windowH);
        if (canvasW > 0 && canvasH > 0) {
            if (windowW != int(canvasW) || windowH != int(canvasH)) SDL_SetWindowSize(window, int(canvasW), int(canvasH));
            int logicalW = 0, logicalH = 0;
            port::platform::turn::size(window, &logicalW, &logicalH);
            const float nextScale = webEditorScale(float(logicalW), float(logicalH));
            if (nextScale != scale) {
                editor.uiScale(nextScale);
                scale = nextScale;
                mcui::ApplyStyle(scale);
                ImGui::GetStyle().WindowPadding = ImVec2(16 * scale, 16 * scale);
                ImGui::GetStyle().ItemSpacing = ImVec2(8 * scale, 8 * scale);
            }
        }
#endif
        while (SDL_PollEvent(&e)) {
            port::platform::turn::event(&e);
            if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION || e.type == SDL_FINGERUP) {
                int w = 0, h = 0;
                port::platform::turn::size(window, &w, &h);
                editor.finger(e.tfinger, w, h);
            }
            bool mouseFromBrowser = false;
#ifdef __EMSCRIPTEN__
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.which == SDL_TOUCH_MOUSEID)
                ImGui::GetIO().AddMousePosEvent(float(e.button.x), float(e.button.y));
            if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION || e.type == SDL_FINGERUP)
                lastFingerAt = SDL_GetTicks();
            if (lastFingerAt && SDL_GetTicks() - lastFingerAt < 800) {
                if (e.type == SDL_MOUSEMOTION) mouseFromBrowser = e.motion.which != SDL_TOUCH_MOUSEID;
                if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
                    mouseFromBrowser = e.button.which != SDL_TOUCH_MOUSEID;
            }
#endif
            const bool pointer = e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP ||
                                 e.type == SDL_MOUSEWHEEL ||
                                 (e.type == SDL_WINDOWEVENT && (e.window.event == SDL_WINDOWEVENT_ENTER || e.window.event == SDL_WINDOWEVENT_LEAVE ||
                                                                e.window.event == SDL_WINDOWEVENT_FOCUS_LOST));
            if (!mouseFromBrowser && !((demoL17 || demoL19 || demoL33 || (demoL46 && strcmp(demoL46, "drag") == 0)) && pointer)) ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) quit = true;
        }
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        port::platform::turn::imgui(window);
        ImGui::NewFrame();
        editor.frame(&quit);
#ifdef MEDECOMP_ONE_PROCESS
        if (!s_testLevel.empty() || s_service) quit = true;
#endif
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        port::platform::turn::begin(renderer, window);
        const ImVec4 bg = mcui::Color(mcui::tokens::kColorBgPrimary);
        SDL_SetRenderDrawColor(renderer, Uint8(bg.x * 255), Uint8(bg.y * 255), Uint8(bg.z * 255), 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        frames++;
        if (demo && !demoStarted && frames == 3) {
            demoStarted = true;
            bool scratch = getenv("MEDECOMP_EDITOR_DEMO_SCRATCH") != nullptr;
            if (editor.create(demo, "chapter_01", scratch) && !editor.doc.map.starts.empty() && !editor.doc.map.finishes.empty()) {
                Map& m = editor.doc.map;
                Sel finish;
                finish.kind = Kind::Finish;
                moveSel(m, finish, toF(m.starts[0].pos[0]) + 18 - toF(m.finishes[0][0]),
                        toF(m.starts[0].pos[1]) - 2 - toF(m.finishes[0][1]));
                editor.view.cx = toF(m.starts[0].pos[0]) + 8;
                editor.view.cy = toF(m.starts[0].pos[1]) - 1;
                editor.addShape(0, 0);
                editor.test();
                fprintf(stderr, "medecomp: редактор: демо %s — %s (pid %lld)\n", demo, editor.doc.status.c_str(), editor.testPid);
                if (editor.testPid < 0) code = 1, quit = true;
            } else {
                code = 1, quit = true;
            }
        }
        if (demoL8 && l8 == 0 && frames == 3) {
            l8 = editor.create(demoL8, "chapter_01") ? 1 : (code = 1, quit = true, 0);
        } else if (l8 == 1 && !editor.demoWaiting()) {
            if (!editor.demoFill() || !editor.demoPreviewArea()) code = 1, quit = true;
            l8 = 2;
        } else if (l8 == 2 && !editor.demoWaiting()) {
            editor.demoSelectFirstFilled();
            if (!editor.demoMapShown()) code = 1;
            l8 = 3, frames = 0;
        }
        if (demoL17 && !quit) {
            ImGuiIO& dio = ImGui::GetIO();
            auto saveShot = [&](const char* tag) {
                if (!shot) return;
                SDL_Surface* image = SDL_CreateRGBSurfaceWithFormat(0, int(dio.DisplaySize.x * dio.DisplayFramebufferScale.x),
                                                                    int(dio.DisplaySize.y * dio.DisplayFramebufferScale.y), 32,
                                                                    SDL_PIXELFORMAT_ARGB8888);
                if (image && SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, image->pixels, image->pitch) == 0)
                    SDL_SaveBMP(image, (std::string(shot) + tag).c_str());
                if (image) SDL_FreeSurface(image);
            };
            if (l17 == 0 && frames == 3) {
                l17 = editor.create(demoL17, "chapter_01", true) ? 1 : (code = 1, quit = true, 0);
                l17Frame = frames;
            } else if (l17 == 1 && frames == l17Frame + 10) {
                ImVec2 at = editor.demoPoint(6, 1);
                dio.AddMousePosEvent(at.x, at.y);
                dio.AddMouseButtonEvent(ImGuiMouseButton_Right, true);
                l17 = 2, l17Frame = frames;
            } else if (l17 == 2 && frames == l17Frame + 2) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
                l17 = 3, l17Frame = frames;
            } else if (l17 == 3 && frames == l17Frame + 6) {
                saveShot("-menu.bmp");
                ImVec2 a = editor.demoAddPoint(), c = editor.demoViewCenter();
                fprintf(stderr, "medecomp: редактор: демо L17 — меню в точке %.2f %.2f (середина вида %.2f %.2f)\n", a.x, a.y, c.x,
                        c.y);
                editor.demoPickBag();
                l17 = 4, l17Frame = frames;
            } else if (l17 == 4 && frames == l17Frame + 6) {
                saveShot("-added.bmp");
                fprintf(stderr, "medecomp: редактор: демо L17 — %s\n", editor.demoSelected().c_str());
                editor.demoPieceTool();
                ImVec2 at = editor.demoPoint(10, -8);
                dio.AddMousePosEvent(at.x, at.y);
                l17 = 5, l17Frame = frames;
            } else if (l17 == 5 && frames == l17Frame + 6) {
                saveShot("-tool.bmp");
                editor.demoMusic();
                l17 = 6, l17Frame = frames;
            } else if (l17 == 6 && frames == l17Frame + 6) {
                saveShot("-music.bmp");
                quit = true;
            }
        }
        if (demoL33 && !quit) {
            ImGuiIO& dio = ImGui::GetIO();
            const int step = frames - l33Frame;
            auto next = [&](int to) { l33 = to, l33Frame = frames; };
            if (l33 == 0 && frames == 3) {
                l33 = editor.create(demoL33, "chapter_01", true) ? 1 : (code = 1, quit = true, 0);
                l33Frame = frames;
            } else if (l33 == 1 && step == 8) {
                editor.demoResizeBox();
                fprintf(stderr, "medecomp: редактор: демо L33 — коробка %s\n", editor.demoBoxText().c_str());
                next(2);
            } else if (l33 == 2 && step == 3) {
                l33Mouse = editor.demoBoxPoint(1, 0.5f);
                dio.AddMousePosEvent(l33Mouse.x, l33Mouse.y);
                next(3);
            } else if (l33 == 3 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                next(4);
            } else if (l33 == 4 && step == 3) {
                l33Mouse.x += 100 * scale;
                dio.AddMousePosEvent(l33Mouse.x, l33Mouse.y);
                next(5);
            } else if (l33 == 5 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                next(6);
            } else if (l33 == 6 && step == 3) {
                fprintf(stderr, "medecomp: редактор: демо L33 — правый край на 100 точек: %s\n", editor.demoBoxText().c_str());
                l33Mouse = editor.demoBoxPoint(0, 1);
                dio.AddMousePosEvent(l33Mouse.x, l33Mouse.y);
                next(7);
            } else if (l33 == 7 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                next(8);
            } else if (l33 == 8 && step == 3) {
                l33Mouse = ImVec2(l33Mouse.x - 60 * scale, l33Mouse.y - 60 * scale);
                dio.AddMousePosEvent(l33Mouse.x, l33Mouse.y);
                next(9);
            } else if (l33 == 9 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                next(10);
            } else if (l33 == 10 && step == 3) {
                fprintf(stderr, "medecomp: редактор: демо L33 — левый верхний угол на 60 точек: %s\n", editor.demoBoxText().c_str());
                l33Mouse = editor.demoBoxPoint(1, 1);
                dio.AddMousePosEvent(l33Mouse.x, l33Mouse.y);
                next(11);
                frames = 0;
            }
        }
        if (demoL19 && !quit) {
            ImGuiIO& dio = ImGui::GetIO();
            auto center = [&]() { return editor.demoViewCenter(); };
            auto report = [&](const char* what, ImVec2 was, float ex, float ey) {
                ImVec2 c = center();
                fprintf(stderr, "medecomp: редактор: демо L19 %s — середина вида сдвинулась на %.2f %.2f (ждали %.2f %.2f)\n", what,
                        c.x - was.x, c.y - was.y, ex, ey);
            };
            if (l19 == 0 && frames == 3) {
                l19 = editor.create(demoL19, "chapter_01", true) ? 1 : (code = 1, quit = true, 0);
                l19Frame = frames;
            } else if (l19 == 1 && frames == l19Frame + 8) {
                editor.demoHand();
                l19Mouse = editor.demoPoint(4, -6);
                dio.AddMousePosEvent(l19Mouse.x, l19Mouse.y);
                l19 = 2, l19Frame = frames;
            } else if (l19 == 2 && frames == l19Frame + 6) {
                if (shot) {
                    SDL_Surface* image = SDL_CreateRGBSurfaceWithFormat(0, int(dio.DisplaySize.x * dio.DisplayFramebufferScale.x),
                                                                        int(dio.DisplaySize.y * dio.DisplayFramebufferScale.y), 32,
                                                                        SDL_PIXELFORMAT_ARGB8888);
                    if (image && SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, image->pixels, image->pitch) == 0)
                        SDL_SaveBMP(image, (std::string(shot) + "-hand.bmp").c_str());
                    if (image) SDL_FreeSurface(image);
                }
                l19Center = center();
                dio.AddMousePosEvent(l19Mouse.x, l19Mouse.y);
                dio.AddMouseWheelEvent(0, -2);
                l19 = 3, l19Frame = frames;
            } else if (l19 == 3 && frames == l19Frame + 4) {
                report("колесо", l19Center, 0, -2 * 48 * scale / editor.demoZoom());
                l19Center = center();
                dio.AddMousePosEvent(l19Mouse.x, l19Mouse.y);
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                l19 = 4, l19Frame = frames;
            } else if (l19 == 4 && frames == l19Frame + 3) {
                l19Mouse = ImVec2(l19Mouse.x + 120 * scale, l19Mouse.y + 60 * scale);
                dio.AddMousePosEvent(l19Mouse.x, l19Mouse.y);
                l19 = 5, l19Frame = frames;
            } else if (l19 == 5 && frames == l19Frame + 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                l19 = 6, l19Frame = frames;
            } else if (l19 == 6 && frames == l19Frame + 3) {
                report("«Рука»", l19Center, -120 * scale / editor.demoZoom(), 60 * scale / editor.demoZoom());
                fprintf(stderr, "medecomp: редактор: демо L19 — %s\n", editor.demoSelected().c_str());
                quit = true;
            }
        }
        if (demoL20 && !quit) {
            if (l20 == 0 && frames == 3) {
                const char* chapter = getenv("MEDECOMP_EDITOR_DEMO_CHAPTER");
                l20 = editor.create(demoL20, chapter && *chapter ? chapter : "chapter_01", true) ? 1 : (code = 1, quit = true, 0);
            } else if (l20 == 1 && frames > 10 && !editor.demoWaiting()) {
                const char* piece = getenv("MEDECOMP_EDITOR_DEMO_PIECE");
                l20 = editor.demoCover(piece ? atoi(piece) : 0) ? 2 : (code = 1, quit = true, 0);
            } else if (l20 == 2 && !editor.demoWaiting()) {
                if (!editor.demoMapShown()) code = 1;
                l20 = 3, frames = 0;
            }
        }
        if (demoL41 && !quit) {
            if (l41 == 0 && frames == 3) {
                const char* chapter = getenv("MEDECOMP_EDITOR_DEMO_CHAPTER");
                l41 = editor.create(demoL41, chapter && *chapter ? chapter : "chapter_01", true) ? 1 : (code = 1, quit = true, 0);
            } else if (l41 == 1 && frames > 10 && !editor.demoWaiting()) {
                const char* piece = getenv("MEDECOMP_EDITOR_DEMO_PIECE");
                l41 = editor.demoSurface(piece ? atoi(piece) : 0) ? 3 : (code = 1, quit = true, 0);
                frames = 0;
            }
        }
        if (demoL49 && !quit) {
            if (l49 == 0 && frames == 3) {
                const char* base = getenv("MEDECOMP_EDITOR_DEMO_CHAPTER");
                l49 = editor.create(demoL49, base && *base ? base : "multiplayer_01", getenv("MEDECOMP_EDITOR_DEMO_SCRATCH") != nullptr)
                          ? 1
                          : (code = 1, quit = true, 0);
            } else if (l49 == 1 && frames > 10 && !editor.demoWaiting()) {
                editor.demoFit(nullptr);
                l49 = 2, frames = 0;
            }
        }
        if (demoL52 && !quit && l52 == 0 && editor.demoProgress() >= 10) {
            editor.demoPieceTool();
            fprintf(stderr, "medecomp: редактор: демо L52 — картинок кусков готово %d\n", editor.demoProgress());
            l52 = 1, frames = 0;
        }
        if (demoL48 && !quit && l48 == 0 && frames > 10 && !editor.demoWaiting()) {
            l48 = editor.demoMirror() ? 1 : (code = 1, quit = true, 0);
            frames = 0;
        }
        if (demoL47 && !quit && l47 == 0 && frames > 10 && !editor.demoWaiting()) {
            editor.demoGroups();
            l47 = 1, frames = 0;
        }
        if (demoL64 && !quit && l64 == 0 && (!demoL49 || l49 == 2) && frames > 3 && !editor.demoWaiting()) {
            const int i = editor.demoCheckpointAtStart(float(atof(demoL64)));
            fprintf(stderr, "medecomp: редактор: демо L64 — первый чекпоинт: %s\n", editor.demoDish(i).c_str());
            const std::string error = editor.demoSave();
            fprintf(stderr, "medecomp: редактор: демо L64 — записан: %s\n", error.empty() ? "да" : error.c_str());
            editor.demoShowCheckpoint(i);
            l64 = 1, frames = 0;
        }
        if (demoL63 && !quit && l63 == 0 && frames > 10 && !editor.demoWaiting()) {
            float dx = 8, dy = 3;
            sscanf(demoL63, "%f,%f", &dx, &dy);
            fprintf(stderr, "medecomp: редактор: демо L63 — поставлен: %s\n", editor.demoSwing(dx, dy).c_str());
            const std::string error = editor.demoSave();
            fprintf(stderr, "medecomp: редактор: демо L63 — записан: %s\n", error.empty() ? "да" : error.c_str());
            l63 = 1, frames = 0;
        }
        if (demoL62 && !quit && l62 == 0 && frames > 10 && !editor.demoWaiting() && strncmp(demoL62, "show:", 5) == 0) {
            editor.demoShowCheckpoint(atoi(demoL62 + 5));
            l62 = 1, frames = 0;
        }
        if (demoL62 && !quit && l62 == 0 && frames > 10 && !editor.demoWaiting()) {
            const int i = atoi(demoL62);
            auto say = [&](const char* what, const std::string& text) { fprintf(stderr, "medecomp: редактор: демо L62 — %s: %s\n", what, text.c_str()); };
            say("было", editor.demoDish(i));
            editor.demoOwnDish(i);
            say("своя тарелка", editor.demoDish(i));
            const char* comma = strchr(demoL62, ',');
            const int fresh = editor.demoCheckpointAtStart(comma ? float(atof(comma + 1)) : 0.0f);
            say("новый у старта", editor.demoDish(fresh));
            say("удалить его тарелку", editor.demoDeleteDish(fresh));
            const std::string error = editor.demoSave();
            say("записан", error.empty() ? "да" : error);
            say("после записи", editor.demoDish(fresh));
            editor.demoShowCheckpoint(fresh);
            l62 = 1, frames = 0;
        }
        if (demoL59 && !quit) {
            ImGuiIO& dio = ImGui::GetIO();
            const int step = frames - l59Frame;
            auto next = [&](int to) { l59 = to, l59Frame = frames; };
            float x = 0, y = 0;
            sscanf(demoL59, "%f,%f", &x, &y);
            const ImVec2 at = editor.demoScreen(x, y);
            if (l59 == 0 && frames > 10 && !editor.demoWaiting()) {
                char region[96];
                snprintf(region, sizeof region, "%g,%g,%g,%g", x - 10, y - 6, x + 10, y + 6);
                editor.demoFit(region);
                next(1);
            } else if (l59 >= 1 && l59 <= 10 && step == 3) {
                if (l59 % 2) dio.AddMousePosEvent(at.x, at.y);
                if (l59 % 2 && l59 > 1)
                    fprintf(stderr, "medecomp: редактор: демо L59 — щелчок %d: %s\n", l59 / 2, editor.demoSelName().c_str());
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, l59 % 2 == 1);
                next(l59 + 1);
            } else if (l59 == 11 && step == 3) {
                fprintf(stderr, "medecomp: редактор: демо L59 — щелчок 5: %s\n", editor.demoSelName().c_str());
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                next(12);
            } else if (l59 == 12 && step == 3) {
                dio.AddMousePosEvent(at.x + 80 * scale, at.y);
                next(13);
            } else if (l59 == 13 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                next(14);
            } else if (l59 == 14 && step == 3) {
                fprintf(stderr, "medecomp: редактор: демо L59 — протянуть выбранное: %s\n", editor.demoSelName().c_str());
                dio.AddMousePosEvent(at.x + 80 * scale, at.y);
                next(15);
                frames = 0;
            }
        }
        if (demoL58 && !quit) {
            ImGuiIO& dio = ImGui::GetIO();
            const int step = frames - l58Frame;
            auto next = [&](int to) { l58 = to, l58Frame = frames; };
            auto say = [&](const char* what) { fprintf(stderr, "medecomp: редактор: демо L58 — %s: %s\n", what, editor.demoGroupText().c_str()); };
            float a[4] = {};
            sscanf(demoL58, "%f,%f,%f,%f", &a[0], &a[1], &a[2], &a[3]);
            if (l58 == 0 && frames > 10 && !editor.demoWaiting()) {
                char region[96];
                snprintf(region, sizeof region, "%g,%g,%g,%g", a[0] - 6, a[1] - 6, a[2] + 6, a[3] + 6);
                editor.demoFit(region);
                next(1);
            } else if (l58 == 1 && step == 3) {
                if (!editor.demoGroupArea(a)) code = 1, quit = true;
                say("в области");
                const std::string before = editor.demoCounts();
                editor.demoDeleteGroup();
                const std::string after = editor.demoCounts();
                editor.demoUndo();
                fprintf(stderr, "medecomp: редактор: демо L58 — удалить группу: %s → %s, отменить: %s\n", before.c_str(), after.c_str(),
                        editor.demoCounts().c_str());
                editor.demoGroupArea(a);
                l58Mouse = editor.demoGroupPoint(Kind::Node);
                dio.AddMousePosEvent(l58Mouse.x, l58Mouse.y);
                next(2);
            } else if (l58 == 2 && step == 1) {
                dio.AddKeyEvent(ImGuiMod_Ctrl, true);
            } else if (l58 == 2 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                next(3);
            } else if (l58 == 3 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                dio.AddKeyEvent(ImGuiMod_Ctrl, false);
                next(4);
            } else if (l58 == 4 && step == 1) {
                say("Ctrl+щелчок по куску");
                dio.AddKeyEvent(ImGuiMod_Ctrl, true);
            } else if (l58 == 4 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                next(5);
            } else if (l58 == 5 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                dio.AddKeyEvent(ImGuiMod_Ctrl, false);
                next(6);
            } else if (l58 == 6 && step == 3) {
                say("ещё Ctrl+щелчок");
                l58Mouse = editor.demoGroupPoint(Kind::Shape);
                dio.AddMousePosEvent(l58Mouse.x, l58Mouse.y);
                next(7);
            } else if (l58 == 7 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                next(8);
            } else if (l58 == 8 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                next(9);
            } else if (l58 == 9 && step == 3) {
                say("щелчок по коробке");
                editor.demoGroupArea(a);
                l58Mouse = editor.demoGroupPoint(Kind::Shape);
                dio.AddMousePosEvent(l58Mouse.x, l58Mouse.y);
                next(10);
            } else if (l58 == 10 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                next(11);
            } else if (l58 == 11 && step == 3) {
                l58Mouse.x += 120 * scale;
                dio.AddMousePosEvent(l58Mouse.x, l58Mouse.y);
                next(12);
            } else if (l58 == 12 && step == 3) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                next(13);
            } else if (l58 == 13 && step == 3) {
                say("протянуть группу");
                next(14);
                frames = 0;
            }
        }
        if (demoL46 && !quit) {
            ImGuiIO& dio = ImGui::GetIO();
            const bool drag = strcmp(demoL46, "drag") == 0;
            if (l46 == 0 && frames > 10 && !editor.demoWaiting()) {
                l46 = editor.demoDepth(demoL46) ? (drag ? 1 : 9) : (code = 1, quit = true, 0);
                frames = 0;
            } else if (l46 == 1 && frames == 4) {
                l46Mouse = editor.demoDepthPoint();
                dio.AddMousePosEvent(l46Mouse.x, l46Mouse.y);
                l46 = 2;
            } else if (l46 == 2 && frames == 7) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                l46 = 3;
            } else if (l46 == 3 && frames >= 10 && frames < 14) {
                l46Mouse.y -= 10 * io.DisplayFramebufferScale.y;
                dio.AddMousePosEvent(l46Mouse.x, l46Mouse.y);
            } else if (l46 == 3 && frames == 16) {
                dio.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                l46 = 4;
            } else if (l46 == 4 && frames == 19) {
                fprintf(stderr, "medecomp: редактор: демо L46 — протянут вверх на 40 точек: z %.2f\n", editor.demoDepthZ());
                l46 = 9, frames = 0;
            }
        }
        if (demoL45 && !quit && l45 == 0 && frames > 10 && !editor.demoWaiting()) {
            editor.demoFit(strchr(demoL45, ',') ? demoL45 : nullptr);
            l45 = 1, frames = 0;
        }
        if (demoL44 && !quit && l44 == 0 && frames == 3) {
            l44 = editor.demoSurfacePopup(strcmp(demoL44, "cover") == 0) ? 1 : (code = 1, quit = true, 0);
            frames = 0;
        }
        if (demoL29 && !quit && l29 == 0 && frames > 10 && !editor.demoWaiting()) {
            l29 = editor.demoSelectBlock() ? 1 : (code = 1, quit = true, 0);
            frames = 0;
        }
        if (shot && frames == 6 && (!demoL8 || l8 == 3) && (!demoL20 || l20 == 3) && (!demoL29 || l29 == 1) && (!demoL41 || l41 == 3) && (!demoL44 || l44 == 1) && (!demoL45 || l45 == 1) && (!demoL46 || l46 == 9) && (!demoL47 || l47 == 1) && (!demoL48 || l48 == 1) && (!demoL52 || l52 == 1) && (!demoL49 || l49 == 2) && (!demoL58 || l58 == 14) && (!demoL59 || l59 == 15) && (!demoL62 || l62 == 1) && (!demoL63 || l63 == 1) && (!demoL64 || l64 == 1) && (!demoL33 || l33 == 11) &&
            !demoL17 && !demoL19) {
            SDL_Surface* image = SDL_CreateRGBSurfaceWithFormat(0, io.DisplaySize.x * io.DisplayFramebufferScale.x,
                                                                io.DisplaySize.y * io.DisplayFramebufferScale.y, 32, SDL_PIXELFORMAT_ARGB8888);
            if (image && SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, image->pixels, image->pitch) == 0)
                SDL_SaveBMP(image, shot);
            if (image) SDL_FreeSurface(image);
            if (!demo) quit = true;
            if (demoL8) quit = true;
        }
        port::platform::turn::end(renderer, window);
        SDL_RenderPresent(renderer);
        if (!childRunning(editor.testPid) && demoStarted) quit = true;
    }
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    port::platform::turn::release(renderer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return code;
}

#ifdef MEDECOMP_ONE_PROCESS
bool takeTestLevel(std::string* id) {
    if (s_testLevel.empty()) return false;
    *id = s_testLevel;
    s_testLevel.clear();
    return true;
}

bool takeServiceRun(std::vector<std::string>* env, std::string* level) {
    if (!s_service) return false;
    s_service = false;
    *env = s_serviceEnv;
    *level = s_serviceLevel;
    return true;
}

bool takeResume(std::string* id) {
    FILE* f = fopen(resumePath().c_str(), "rb");
    if (!f) return false;
    char level[256] = {}, what[16] = {};
    Resume r;
    bool ok = fscanf(f, "%255s %15s %f %f %f %d %f %f %f %f", level, what, &r.cx, &r.cy, &r.zoom, &r.hasArea, &r.area[0],
                     &r.area[1], &r.area[2], &r.area[3]) == 10;
    fclose(f);
    remove(resumePath().c_str());
    if (!ok || !*level) return false;
    r.valid = true;
    r.what = what;
    s_resume = r;
    *id = level;
    return true;
}
#endif

}
}
