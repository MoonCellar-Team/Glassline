#include "level.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <type_traits>

#include "../platform/levels.h"
#include "port/files.h"

namespace port {
namespace editor {

Fix toFix(float v) { return Fix(lroundf(v * 65536.0f)); }

namespace {

const float kNodeMargin = 8.0f;

class IO {
public:
    explicit IO(const std::vector<unsigned char>* in) : m_in(in) {}
    bool reading() const { return m_in != nullptr; }
    bool ok() const { return m_ok; }
    size_t pos() const { return m_pos; }
    void fail() { m_ok = false; }
    std::vector<unsigned char> out;

    template <class T>
    void v(T& x) {
        static_assert(std::is_integral<T>::value && sizeof(T) <= 4, "целое до 4 байт");
        using U = typename std::make_unsigned<T>::type;
        U u = U(x);
        raw(&u, sizeof(T));
        x = T(u);
    }
    void v(float& x) {
        uint32_t u;
        memcpy(&u, &x, 4);
        raw(&u, 4);
        memcpy(&x, &u, 4);
    }
    template <class T, size_t N>
    void arr(T (&a)[N]) {
        for (auto& x : a) v(x);
    }
    template <class T, size_t N>
    void arr(std::array<T, N>& a) {
        for (auto& x : a) v(x);
    }
    template <class C, class V, class F>
    void list(std::vector<V>& items, F fn) {
        C n = C(items.size());
        v(n);
        if (reading()) {
            if (int64_t(n) < 0) fail();
            items.assign(m_ok ? size_t(n) : 0, V());
        }
        for (auto& e : items) {
            if (!m_ok) break;
            fn(e);
        }
    }

private:
    template <class U>
    void raw(U* u, size_t size) {
        if (!reading()) {
            for (size_t i = size; i-- > 0;) out.push_back((uint64_t(*u) >> (8 * i)) & 0xff);
            return;
        }
        if (m_pos + size > m_in->size()) {
            m_ok = false;
            m_pos = m_in->size();
            *u = 0;
            return;
        }
        uint64_t x = 0;
        for (size_t i = 0; i < size; i++) x = x << 8 | (*m_in)[m_pos++];
        *u = U(x);
    }

    const std::vector<unsigned char>* m_in;
    size_t m_pos = 0;
    bool m_ok = true;
};

void ref(IO& io, Ref& r) {
    io.v(r.layer);
    io.v(r.column);
    io.v(r.section);
    io.v(r.node);
}

void visit(IO& io, Map& m) {
    io.v(m.palette);
    io.v(m.unk004);
    io.v(m.modelSet);
    io.v(m.minX);
    io.v(m.maxX);
    io.v(m.sectionWidth);
    io.v(m.numSections);
    if (io.reading()) {
        if (m.numSections < 1) io.fail();
        m.strips.assign(io.ok() ? size_t(m.numSections) * kNumLayers : 0, {});
    }
    for (auto& s : m.strips) {
        io.v(s.first);
        io.v(s.second);
    }
    if (io.reading()) m.shapes.assign(m.strips.empty() ? 0 : size_t(std::max<int>(0, m.strips.back().second)), Shape());
    for (auto& s : m.shapes) {
        io.arr(s.box);
        io.v(s.type);
        io.v(s.material);
    }
    io.list<int8_t>(m.layers, [&](Layer& l) {
        io.list<int8_t>(l.columns, [&](Column& c) {
            io.arr(c.bounds);
            io.list<int8_t>(c.sections, [&](Section& s) {
                io.arr(s.bounds);
                io.v(s.visionSlots);
                io.list<int8_t>(s.nodes, [&](Node& n) {
                    int32_t id = n.id | (n.mirror ? kNodeMirror : 0);
                    io.v(id);
                    if (io.reading()) n.mirror = (id & kNodeMirror) != 0, n.id = id & ~kNodeMirror;
                    io.arr(n.pos);
                    io.v(n.shared);
                    io.v(n.visionFlag);
                    if (n.visionFlag == 1) {
                        io.v(n.visionType);
                        io.arr(n.visionBox);
                    }
                });
            });
        });
    });
    io.list<int16_t>(m.paths, [&](std::vector<PathPoint>& p) {
        io.list<int16_t>(p, [&](PathPoint& q) {
            io.arr(q.pos);
            io.v(q.action);
            io.v(q.speed);
        });
    });
    io.list<int16_t>(m.fallBoxes, [&](std::array<float, 6>& b) { io.arr(b); });
    io.list<int16_t>(m.painBoxes, [&](std::array<float, 6>& b) { io.arr(b); });
    io.list<uint16_t>(m.objects, [&](Object& o) {
        io.v(o.type);
        io.arr(o.pos);
        io.v(o.path);
        io.v(o.refFlag);
        if (o.refFlag == 1) ref(io, o.ref);
    });
    io.list<uint8_t>(m.starts, [&](Start& s) {
        io.arr(s.pos);
        io.v(s.facing);
    });
    io.list<uint8_t>(m.finishes, [&](std::array<Fix, 4>& f) { io.arr(f); });
    io.list<uint8_t>(m.checkpoints, [&](Checkpoint& c) {
        io.arr(c.box);
        ref(io, c.ref);
    });
    auto sound = [&](Sound& s) {
        io.v(s.res);
        io.arr(s.pos);
    };
    io.list<uint16_t>(m.ambientSounds, sound);
    io.list<uint16_t>(m.oneShots, sound);
    io.list<uint16_t>(m.musicCues, sound);
    io.list<int8_t>(m.zipLines, [&](std::array<Fix, 5>& z) { io.arr(z); });
    auto area = [&](Area& a) {
        io.v(a.index);
        io.arr(a.box);
    };
    io.list<int16_t>(m.popups, area);
    io.list<int16_t>(m.cutsceneTriggers, area);
    io.list<int16_t>(m.cameraAnims, [&](CameraAnim& c) {
        io.v(c.target);
        io.list<int16_t>(c.steps, [&](CameraStep& s) {
            io.v(s.travel);
            io.v(s.hold);
            io.arr(s.from);
            io.arr(s.at);
        });
    });
}

}

bool readMap(const std::vector<unsigned char>& data, Map* map, std::string* error) {
    IO io(&data);
    *map = Map();
    visit(io, *map);
    if (!io.ok() || io.pos() != data.size()) {
        *error = io.ok() ? "лишние байты в конце" : "файл обрывается";
        return false;
    }
    int n = map->numSections;
    for (int layer = kNumLayers - 1; layer >= 0; layer--)
        for (int i = 0; i < n; i++) {
            auto range = map->strips[size_t(layer * n + i)];
            for (int k = std::max<int>(0, range.first); k < range.second && k < int(map->shapes.size()); k++)
                map->shapes[size_t(k)].layer = layer;
        }
    return true;
}

std::vector<unsigned char> writeMap(const Map& map) {
    IO io(nullptr);
    visit(io, const_cast<Map&>(map));
    return io.out;
}

void rebuildStrips(Map* m) {
    for (auto& s : m->shapes) s.layer = std::max(0, std::min(kNumLayers - 1, s.layer));
    std::stable_sort(m->shapes.begin(), m->shapes.end(), [](const Shape& a, const Shape& b) {
        return a.layer != b.layer ? a.layer < b.layer : a.box[0] < b.box[0];
    });
    Fix lo = m->minX, hi = m->maxX;
    for (auto& s : m->shapes) lo = std::min(lo, s.box[0]), hi = std::max(hi, s.box[2]);
    if (lo < m->minX) m->minX = toFix(floorf(toF(lo)) - 1);
    if (hi > m->maxX) m->maxX = toFix(ceilf(toF(hi)) + 1);
    if (m->sectionWidth <= 0) m->sectionWidth = toFix(64);
    float width = toF(m->sectionWidth);
    int n = std::max(1, int(ceilf((toF(m->maxX) - toF(m->minX)) / width)));
    m->numSections = int16_t(n);
    m->strips.assign(size_t(n) * kNumLayers, {});
    size_t first = 0;
    for (int layer = 0; layer < kNumLayers; layer++) {
        size_t end = first;
        while (end < m->shapes.size() && m->shapes[end].layer == layer) end++;
        int prev = int(first);
        for (int i = 0; i < n; i++) {
            float x0 = toF(m->minX) + i * width, x1 = x0 + width;
            int a = -1, b = -1;
            for (size_t k = first; k < end; k++) {
                const Shape& s = m->shapes[k];
                if (toF(s.box[0]) < x1 && toF(s.box[2]) > x0) {
                    if (a < 0) a = int(k);
                    b = int(k) + 1;
                }
            }
            if (a < 0) a = b = prev;
            b = std::max(b, prev);
            m->strips[size_t(layer * n + i)] = {int16_t(a), int16_t(b)};
            prev = b;
        }
        first = end;
    }
    if (!m->strips.empty()) m->strips.back().second = int16_t(m->shapes.size());
}

void growBounds(Map* m) {
    Fix margin = toFix(kNodeMargin);
    for (auto& layer : m->layers)
        for (auto& column : layer.columns) {
            for (auto& section : column.sections) {
                for (auto& node : section.nodes)
                    for (int a = 0; a < 2; a++) {
                        section.bounds[a] = std::min(section.bounds[a], node.pos[a] - margin);
                        section.bounds[3 + a] = std::max(section.bounds[3 + a], node.pos[a] + margin);
                    }
                for (int a = 0; a < 3; a++) {
                    column.bounds[a] = std::min(column.bounds[a], section.bounds[a]);
                    column.bounds[3 + a] = std::max(column.bounds[3 + a], section.bounds[3 + a]);
                }
            }
        }
}

int selfCheck() {
    std::string res = port::files::bundle() + "/res";
    int maps = 0, bad = 0;
    for (const std::string& name : port::files::list(res)) {
        bool level = name.size() > 4 && name.compare(name.size() - 4, 4, ".bin") == 0 &&
                     (name.compare(0, 8, "chapter_") == 0 || name.compare(0, 12, "multiplayer_") == 0 ||
                      name.compare(0, 8, "tutorial") == 0);
        if (!level) continue;
        maps++;
        std::vector<unsigned char> data;
        Map map;
        std::string error;
        if (!port::files::read(res + "/" + name, &data) || !readMap(data, &map, &error)) {
            printf("%s: не прочитана: %s\n", name.c_str(), error.c_str());
            bad++;
            continue;
        }
        bool same = writeMap(map) == data;
        Map rebuilt = map;
        rebuildStrips(&rebuilt);
        int n = rebuilt.numSections, gaps = 0;
        float width = toF(rebuilt.sectionWidth);
        for (int layer = 0; layer < kNumLayers; layer++)
            for (int i = 0; i < n; i++) {
                auto r = rebuilt.strips[size_t(layer * n + i)];
                if (i > 0) {
                    auto p = rebuilt.strips[size_t(layer * n + i - 1)];
                    if (r.first < p.first || r.second < p.second) gaps++;
                }
                float x0 = toF(rebuilt.minX) + i * width, x1 = x0 + width;
                for (size_t k = 0; k < rebuilt.shapes.size(); k++) {
                    const Shape& s = rebuilt.shapes[k];
                    if (s.layer == layer && toF(s.box[0]) < x1 && toF(s.box[2]) > x0 && (int(k) < r.first || int(k) >= r.second))
                        gaps++;
                }
            }
        std::string check = rebuilt.finishes.empty() ? std::string() : port::platform::checkPlayerMap(writeMap(rebuilt));
        bool ok = same && gaps == 0 && check.empty();
        bad += !ok;
        printf("%s: %s, полосы заново: %s, проверка: %s\n", name.c_str(), same ? "байт в байт" : "НЕ СОВПАЛО",
               gaps ? "ДЫРЫ" : "покрывают формы", check.empty() ? "годна" : check.c_str());
    }
    printf("карт %d, с ошибками %d\n", maps, bad);
    return maps && !bad ? 0 : 1;
}

}
}
