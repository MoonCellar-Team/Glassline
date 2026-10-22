#include "levels.h"
#include "piece_surface.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>

#include "../ea/json.h"
#include "../ea/levels.h"
#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/ResourceManager.h"
#include "port/cf.h"
#include "port/files.h"
#include "runtime/midp/ByteArrayInputStream.h"
#include "runtime/midp/String.h"

extern "C" int medecomp_map_table_size(int table);
extern "C" void medecomp_quit_game(void);
extern "C" void medecomp_back_to_launcher(void);

namespace {

const size_t kMaxMap = 4 << 20;
const int kSceneMenu = 2;
const int kSceneGame = 3;

std::vector<unsigned char> s_map;
std::string s_id, s_palette;
std::string s_name;
bool s_bags;
int s_timeLimit;
bool s_solo;
bool s_supports;
bool s_pending;
int s_base = -1;
int s_best = -1;
bool s_entered;
bool s_test;

struct Reader {
    const std::vector<unsigned char>& d;
    size_t pos = 0;
    std::string error;

    explicit Reader(const std::vector<unsigned char>& data) : d(data) {}
    unsigned bytes(int n) {
        if (pos + n > d.size()) {
            if (error.empty()) error = "файл обрывается на байте " + std::to_string(pos);
            pos = d.size();
            return 0;
        }
        unsigned v = 0;
        for (int i = 0; i < n; i++) v = v << 8 | d[pos++];
        return v;
    }
    int i8() { return int8_t(bytes(1)); }
    int u8() { return int(bytes(1)); }
    int i16() { return int16_t(bytes(2)); }
    int u16() { return int(bytes(2)); }
    int i32() { return int32_t(bytes(4)); }
    void skip(long n) {
        if (n >= 0 && pos + size_t(n) <= d.size()) {
            pos += size_t(n);
        } else {
            if (error.empty()) error = "файл обрывается на байте " + std::to_string(pos);
            pos = d.size();
        }
    }
    bool fail(const std::string& what) {
        if (error.empty()) error = what + " (байт " + std::to_string(pos) + ")";
        return false;
    }
    bool index(int v, int table, const char* what) {
        return v >= 0 && v < medecomp_map_table_size(table) ? true : fail(std::string(what) + " " + std::to_string(v) + " вне таблицы");
    }
    int count(int n, const char* what, int min = 0) {
        if (n < min) fail(std::string(what) + ": " + std::to_string(n));
        return std::max(0, n);
    }
};

struct Section {
    int nodes, visions;
};
using Layers = std::vector<std::vector<std::vector<Section>>>;

bool readRef(Reader& r, const Layers& layers, bool checkpoint) {
    int l = r.i16(), c = r.i16(), s = r.i16(), n = r.i16();
    if (l < 0 || l >= int(layers.size()) || c < 0 || c >= int(layers[l].size()) || s < 0 ||
        s >= int(layers[l][c].size()) || n < 0 || n >= layers[l][c][s].nodes)
        return r.fail("ссылка на узел чанка вне графики");
    if (checkpoint && n >= layers[l][c][s].visions) return r.fail("чекпоинт ссылается на узел без подсветки");
    return true;
}

port::platform::LevelMode modeOf(const port::online::Json& info) {
    port::platform::LevelMode mode;
    mode.bags = info["mode"].str() == "bags";
    if (!mode.bags) return mode;
    long long limit = info["timeLimit"].integer();
    mode.timeLimit = port::platform::kBagTimeLimitDefault;
    for (int allowed : port::platform::kBagTimeLimits)
        if (limit == allowed) mode.timeLimit = allowed;
    return mode;
}

}

namespace port {
namespace platform {

std::string checkPlayerMap(const std::vector<unsigned char>& data, bool bags) {
    Reader r(data);
    if (data.size() > kMaxMap) return "карта больше " + std::to_string(kMaxMap >> 20) + " МБ";
    r.index(r.i32(), 0, "палитра");
    r.i8();
    r.index(r.i32(), 1, "набор моделей");
    r.skip(12);
    int strips = r.count(r.i16(), "полос форм", 1);
    std::vector<std::pair<int, int>> ranges;
    for (int i = 0; i < strips * 4 && r.error.empty(); i++) ranges.push_back({r.i16(), r.i16()});
    int shapes = ranges.empty() ? 0 : ranges.back().second;
    for (auto& range : ranges)
        if (range.first < 0 || range.first > range.second || range.second > shapes) r.fail("полоса форм вне списка форм");
    for (int i = 0; i < shapes && r.error.empty(); i++) {
        r.skip(16);
        int type = r.i8();
        if (type < 0 || type > 2) r.fail("тип формы " + std::to_string(type));
        const int material = r.i32();
        r.index(port::platform::shapeMaterialIndex(material), 2, "материал");
        if (material < 0) r.fail("материал " + std::to_string(material));
    }
    Layers layers(r.count(r.i8(), "слоёв графики"));
    for (auto& layer : layers) {
        layer.resize(r.count(r.i8(), "колонок"));
        for (auto& column : layer) {
            r.skip(24);
            column.resize(r.count(r.i8(), "секций"));
            for (auto& section : column) {
                r.skip(24);
                int slots = r.count(r.i8(), "мест под подсветки");
                section.nodes = r.count(r.i8(), "узлов в секции", 1);
                section.visions = 0;
                for (int n = 0; n < section.nodes && r.error.empty(); n++) {
                    r.skip(17);
                    if (r.i8() == 1) {
                        section.visions++;
                        r.skip(17);
                    }
                }
                if (section.visions > slots) r.fail("подсветок больше, чем мест под них");
            }
        }
    }
    std::vector<int> paths(r.count(r.i16(), "путей"));
    for (int& points : paths) {
        points = r.count(r.i16(), "точек пути");
        r.skip(points * 20L);
    }
    for (int k = 0; k < 2; k++) r.skip(r.count(r.i16(), k ? "зон урона" : "зон падения") * 24L);
    int bagCount = 0;
    for (int n = r.u16(); n > 0 && r.error.empty(); n--) {
        int type = r.i32();
        if (type == 3) bagCount++;
        r.skip(12);
        int path = r.i32();
        bool onPath = type == 1 || type == 4 || type == 5 || type == 7 || type == 8;
        bool needsPath = type == 5 || type == 7;
        bool pathOk = path >= 0 && path < int(paths.size()) && paths[size_t(path)] >= 1;
        if (onPath && !pathOk && (needsPath || path != -1)) r.fail("объект на пути без пути");
        if (!onPath && type != 0 && type != 6) r.index(type, 3, "тип объекта");
        if (r.i8() == 1) readRef(r, layers, false);
    }
    int starts = r.count(r.u8(), "стартов", 1);
    for (int i = 0; i < starts; i++) {
        r.skip(12);
        int facing = r.i8();
        if (facing != -1 && facing != 1) r.fail("направление старта " + std::to_string(facing));
    }
    r.skip(r.count(r.u8(), "финишей", bags ? 0 : 1) * 16L);
    if (bags && bagCount == 0 && r.error.empty()) r.fail("сумок: 0 — сбору сумок нужна хотя бы одна");
    for (int n = r.u8(); n > 0 && r.error.empty(); n--) {
        r.skip(16);
        readRef(r, layers, true);
    }
    for (int k = 0; k < 3; k++)
        for (int n = r.u16(); n > 0 && r.error.empty(); n--) {
            r.index(r.i32(), 0, "звук");
            r.skip(12);
        }
    r.skip(r.count(r.i8(), "тросов") * 20L);
    for (int k = 0; k < 2; k++) {
        for (int n = r.count(r.i16(), k ? "триггеров заставок" : "подсказок"); n > 0 && r.error.empty(); n--) {
            r.index(r.i32(), k ? 5 : 4, k ? "заставка" : "подсказка");
            r.skip(16);
        }
    }
    for (int anims = r.count(r.i16(), "анимаций камеры"); anims > 0 && r.error.empty(); anims--) {
        r.i8();
        r.skip(r.count(r.i16(), "шагов анимации") * 32L);
    }
    if (r.error.empty() && r.pos != data.size())
        r.error = "после анимаций камеры ещё " + std::to_string(data.size() - r.pos) + " байт";
    return r.error;
}

int baseIndexOf(const std::string& palette) {
    LevelData* levels = AppEngine::getLevelData();
    for (int i = 0; i < levels->getLevelNum(); i++) {
        int res = levels->getLevel(i)->m_mapResId;
        if (res >= 0 && palette + ".bin" == ResourceManager::RESOURCE_FILENAMES_LIST[res]) return i;
    }
    if (!arenaBase(palette)) return -1;
    for (int i = 0; i < levels->getLevelNum(); i++) {
        int res = levels->getLevel(i)->m_mapResIdMultiplayer;
        if (res >= 0 && palette + ".bin" == ResourceManager::RESOURCE_FILENAMES_LIST[res]) return i;
    }
    return -1;
}

bool preparePlayerLevel(const std::string& id, std::string* error) {
    bool plain = !id.empty() && id.size() <= 64;
    for (char c : id) plain = plain && (isalnum((unsigned char)c) || c == '-' || c == '_');
    if (!plain) return *error = "id — латиница, цифры, - и _", false;
    std::string dir = port::files::dataDir() + "/levels/" + id;
    std::vector<unsigned char> json, map;
    if (!port::files::read(dir + "/level.json", &json) || !port::files::read(dir + "/level.bin", &map))
        return *error = "нет " + dir + "/level.json или level.bin", false;
    port::online::Json info;
    if (!port::online::parseJson(std::string(json.begin(), json.end()), &info) || info.type != port::online::Json::Object)
        return *error = "level.json — не объект JSON", false;
    s_palette = info["palette"].str();
    if (s_palette.empty()) return *error = "в level.json нет palette (глава копии игры, например chapter_01)", false;
    LevelMode mode = modeOf(info);
    if (arenaBase(s_palette) && !mode.bags) return *error = "на месте арены " + s_palette + " — только сбор сумок", false;
    *error = checkPlayerMap(map, mode.bags);
    if (!error->empty()) return false;
    s_map.swap(map);
    s_id = id;
    s_name = info["name"].str(id);
    std::vector<unsigned char> best;
    s_best = port::files::read(dir + "/best", &best) ? atoi(std::string(best.begin(), best.end()).c_str()) : -1;
    if (s_best <= 0) s_best = -1;
    s_pending = true;
    s_bags = mode.bags;
    s_timeLimit = mode.timeLimit;
    s_solo = true;
    s_supports = port::platform::supportsVisible(info);
    fprintf(stderr, "medecomp: уровень игрока %s — «%s» (%s), на месте %s, %s\n", id.c_str(), info["name"].str().c_str(),
            info["author"].str().c_str(), s_palette.c_str(),
            mode.bags ? ("сбор сумок, " + std::to_string(mode.timeLimit / 1000) + " с").c_str() : "гонка");
    return true;
}

bool supportsVisible(const port::online::Json& info) {
    const std::string supports = info["supports"].str();
    if (supports == "visible") return true;
    if (supports == "hidden") return false;
    return info["origin"].str("chapter") != "chapter";
}

LevelMode readLevelMode(const std::string& id) {
    std::vector<unsigned char> json;
    port::online::Json info;
    if (!port::files::read(port::files::dataDir() + "/levels/" + id + "/level.json", &json) ||
        !port::online::parseJson(std::string(json.begin(), json.end()), &info))
        return LevelMode();
    return modeOf(info);
}

std::string bestBags(const std::string& id) {
    std::vector<unsigned char> text;
    if (!port::files::read(port::files::dataDir() + "/levels/" + id + "/best_bags", &text)) return std::string();
    std::string s(text.begin(), text.end());
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}

bool startPlayerLevelTogether(const std::string& id, int* base, std::string* error, bool solo) {
    if (!preparePlayerLevel(id, error)) return false;
    s_pending = false;
    s_solo = solo;
    s_entered = false;
    s_base = baseIndexOf(s_palette);
    if (s_base < 0) return *error = "главы " + s_palette + " нет в копии игры", false;
    *base = s_base;
    fprintf(stderr, "medecomp: уровень игрока %s — %s, на месте главы %d\n", id.c_str(), solo ? "BAG HUNT" : "вдвоём",
            s_base);
    return true;
}

void clearPlayerLevel() {
    if (s_base >= 0) fprintf(stderr, "medecomp: уровень игрока %s больше не идёт — глава своя\n", s_id.c_str());
    s_base = -1;
    s_pending = false;
    s_entered = false;
}

uint64_t levelHash(const std::vector<unsigned char>& data) {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : data) h = (h ^ c) * 1099511628211ULL;
    return h;
}

bool currentSharedLevel(SharedLevel* out) {
    if (s_base < 0 || s_map.empty() || AppEngine::getLevelData()->getCurrentLevelIndex() != s_base) return false;
    std::vector<unsigned char> json;
    port::online::Json info;
    if (port::files::read(port::files::dataDir() + "/levels/" + s_id + "/level.json", &json))
        port::online::parseJson(std::string(json.begin(), json.end()), &info);
    out->name = info["name"].str(s_id);
    out->author = info["author"].str();
    out->palette = s_palette;
    out->bags = s_bags;
    out->timeLimit = s_bags ? s_timeLimit : 0;
    std::string origin = info["origin"].str();
    bool onServer = origin == "uploaded" || origin == "online";
    out->serverId = onServer ? info["server"]["id"].str() : std::string();
    out->serverVersion = onServer ? int(info["server"]["version"].integer()) : 0;
    out->map = s_map;
    out->hash = levelHash(s_map);
    return true;
}

std::string findLevelByMap(uint64_t hash, size_t size) {
    std::string dir = port::files::dataDir() + "/levels";
    for (const std::string& id : port::files::list(dir)) {
        if (port::files::stat(dir + "/" + id + "/level.bin").size != size) continue;
        std::vector<unsigned char> map;
        if (port::files::read(dir + "/" + id + "/level.bin", &map) && levelHash(map) == hash &&
            port::files::stat(dir + "/" + id + "/level.json").exists)
            return id;
    }
    return std::string();
}

std::string savePeerLevel(const SharedLevel& level, std::string* error) {
    char name[32];
    snprintf(name, sizeof name, "peer-%016llx", (unsigned long long)level.hash);
    std::string root = port::files::dataDir() + "/levels", dir = root + "/" + name;
    mkdir(root.c_str(), 0755);
    mkdir(dir.c_str(), 0755);
    std::string json = "{\n  \"name\": " + port::online::jsonString(level.name) +
                       ",\n  \"author\": " + port::online::jsonString(level.author) +
                       ",\n  \"palette\": " + port::online::jsonString(level.palette) +
                       ",\n  \"version\": 1,\n  \"origin\": \"peer\",\n  \"description\": \"\"" +
                       (level.bags ? ",\n  \"mode\": \"bags\",\n  \"timeLimit\": " + std::to_string(level.timeLimit)
                                   : std::string(",\n  \"mode\": \"race\"")) +
                       "\n}\n";
    FILE* bin = fopen((dir + "/level.bin").c_str(), "wb");
    bool ok = bin && fwrite(level.map.data(), 1, level.map.size(), bin) == level.map.size();
    if (bin) ok = fclose(bin) == 0 && ok;
    FILE* info = ok ? fopen((dir + "/level.json").c_str(), "wb") : nullptr;
    ok = info && fwrite(json.data(), 1, json.size(), info) == json.size();
    if (info) ok = fclose(info) == 0 && ok;
    if (!ok) return *error = "не записать " + dir, std::string();
    return name;
}

std::vector<MultiplayerLevel> multiplayerLevels() {
    std::vector<MultiplayerLevel> out;
    std::string dir = port::files::dataDir() + "/levels";
    for (const std::string& id : port::files::list(dir)) {
        std::vector<unsigned char> json;
        port::online::Json info;
        if (!port::files::read(dir + "/" + id + "/level.json", &json) ||
            !port::online::parseJson(std::string(json.begin(), json.end()), &info) || info.type != port::online::Json::Object ||
            !port::files::stat(dir + "/" + id + "/level.bin").exists)
            continue;
        MultiplayerLevel l;
        l.id = id;
        l.name = info["name"].str(id);
        l.bags = info["mode"].str() == "bags";
        l.base = baseIndexOf(info["palette"].str());
        out.push_back(l);
    }
    std::sort(out.begin(), out.end(), [](const MultiplayerLevel& a, const MultiplayerLevel& b) { return a.name < b.name; });
    return out;
}

void playerLevelFrame() {
    AppEngine* app = AppEngine::getCanvas();
    if (s_base < 0 || !app) return;
    if (app->m_sceneId == kSceneGame) s_entered = true;
    if (s_entered && app->m_sceneId == kSceneMenu) {
        s_base = -1;
        s_entered = false;
        fprintf(stderr, "medecomp: уровень игрока %s закрыт\n", s_id.c_str());
#ifdef __ANDROID__
        if (s_test) medecomp_back_to_launcher();
#else
        if (s_test) medecomp_quit_game();
#endif
    }
}

void setTestRun(bool on) { s_test = on; }
bool testRun() { return s_test; }

}
}

namespace {

bool playing() {
    LevelData* levels = AppEngine::getLevelData();
    return s_base >= 0 && levels->getCurrentLevelIndex() == s_base;
}

bool playingRace() { return playing() && AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_SPEEDRUN; }

bool playingTimed() {
    return playingRace() || (playing() && AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_CHALLENGE);
}

}

extern "C" int medecomp_player_level_base(void) {
    if (!s_pending) return -1;
    s_pending = false;
    s_base = port::platform::baseIndexOf(s_palette);
    if (s_base >= 0) return s_base;
    fprintf(stderr, "medecomp: уровень игрока %s: главы %s в копии игры нет\n", s_id.c_str(), s_palette.c_str());
    return -1;
}

extern "C" void medecomp_player_level_apply(int base) {
    LevelData* levels = AppEngine::getLevelData();
    if (!s_bags) {
        levels->setCurrentLevelByIndex(LevelData::GAME_MODE_SPEEDRUN, base);
        return;
    }
    levels->setCurrentLevelByIndex(LevelData::GAME_MODE_MULTIPLAYER_TIMED, base);
    levels->setLevelTimeLimitMillis(s_timeLimit);
    levels->setMultiplayerCombatAllowed(false);
}

extern "C" int medecomp_player_level_solo(void) {
    return playing() && s_bags && s_solo &&
           AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED;
}

extern "C" void medecomp_player_level_bags(int collected, int total) {
    if (!medecomp_player_level_solo()) return;
    std::string path = port::files::dataDir() + "/levels/" + s_id + "/best_bags";
    std::vector<unsigned char> text;
    int best = port::files::read(path, &text) ? atoi(std::string(text.begin(), text.end()).c_str()) : -1;
    fprintf(stderr, "medecomp: уровень игрока %s: сумок %d из %d%s\n", s_id.c_str(), collected, total,
            collected > best ? " — лучший сбор" : "");
    if (collected <= best) return;
    if (FILE* f = fopen(path.c_str(), "w")) {
        fprintf(f, "%d/%d\n", collected, total);
        fclose(f);
    }
}

extern "C" midp::InputStream* medecomp_player_map(void) {
    if (!playing()) return nullptr;
    midp::array<signed char> bytes(int(s_map.size()));
    memcpy(&bytes[0], s_map.data(), s_map.size());
    return new midp::ByteArrayInputStream(bytes);
}

extern "C" int medecomp_player_level_playing(void) { return playing(); }
extern "C" int medecomp_player_level_supports(void) { return playing() && s_supports; }
extern "C" int medecomp_player_level_test(void) { return s_test; }

extern "C" int medecomp_player_level_record(int timeMs) {
    if (!playingRace()) return 0;
    std::string path = port::files::dataDir() + "/levels/" + s_id + "/best";
    std::vector<unsigned char> text;
    int best = port::files::read(path, &text) ? atoi(std::string(text.begin(), text.end()).c_str()) : 0;
    if (best > 0 && best <= timeMs) return 0;
    if (FILE* f = fopen(path.c_str(), "w")) {
        fprintf(f, "%d\n", timeMs);
        fclose(f);
    }
    s_best = timeMs;
    return 1;
}

extern "C" int medecomp_player_level_no_board(void) {
    return playingRace() && port::platform::playerLevelBoard().empty();
}

extern "C" midp::String* medecomp_player_level_title(void) {
    if (!playing()) return nullptr;
    return new midp::String(static_cast<const __CFString*>(port::cf::makeString(s_name)));
}

extern "C" int medecomp_player_level_best(void) { return playingRace() ? s_best : -1; }

extern "C" const char* medecomp_player_ghost_filename(void) {
    static std::string name;
    if (!playingRace()) return nullptr;
    name = "GhostP-" + s_id;
    return name.c_str();
}

namespace port {
namespace platform {

bool playerLevelPlaying() { return playing(); }
std::string playingBestBags() { return medecomp_player_level_solo() ? bestBags(s_id) : std::string(); }

std::string playerLevelBoard() {
    if (!playingTimed()) return "";
    std::vector<unsigned char> json;
    port::online::Json info;
    if (!port::files::read(port::files::dataDir() + "/levels/" + s_id + "/level.json", &json) ||
        !port::online::parseJson(std::string(json.begin(), json.end()), &info))
        return "";
    return info["server"]["board"].str();
}

}
}

namespace {

enum Text {
    TEXT_CHALLENGE_FAILED = 0x9d8,
    TEXT_CHALLENGE_REMOVED = 0x9d9,
    TEXT_CHALLENGE_UPDATED = 0x9da,
    TEXT_CHALLENGE_OFFLINE = 0x9db,
};

struct ChallengeFetch {
    std::mutex mu;
    int state = 0;
    int text = TEXT_CHALLENGE_FAILED;
    std::string localId;
};
std::shared_ptr<ChallengeFetch> s_fetch;

std::string localByServerId(const std::string& serverId, int want, int* version) {
    std::string dir = port::files::dataDir() + "/levels", found;
    for (const std::string& id : port::files::list(dir)) {
        std::vector<unsigned char> json;
        port::online::Json info;
        if (!port::files::read(dir + "/" + id + "/level.json", &json) ||
            !port::online::parseJson(std::string(json.begin(), json.end()), &info) ||
            info["server"]["id"].str() != serverId || !port::files::stat(dir + "/" + id + "/level.bin").exists)
            continue;
        int v = int(info["server"]["version"].integer());
        if (found.empty() || v == want) found = id, *version = v;
        if (v == want) break;
    }
    return found;
}

void fetchChallengeLevel(std::shared_ptr<ChallengeFetch> fetch, std::string serverId, int want) {
    int have = 0, state = 1, text = 0;
    std::string local = localByServerId(serverId, want, &have);
    if (local.empty() || have != want) {
        port::online::NetLevel level;
        bool missing = false;
        std::string error;
        if (!port::online::getLevel(serverId, &level, &missing, &error)) {
            state = missing ? -1 : -2, text = missing ? TEXT_CHALLENGE_REMOVED : TEXT_CHALLENGE_OFFLINE;
        } else if (level.version != want) {
            state = -1, text = TEXT_CHALLENGE_UPDATED;
        } else if (!port::online::downloadLevel(level, &local, &error)) {
            state = -2, text = TEXT_CHALLENGE_OFFLINE;
        }
    }
    fprintf(stderr, "medecomp: вызов на уровне игрока %s версии %d: %s\n", serverId.c_str(), want,
            state > 0 ? ("уровень " + local).c_str()
            : text == TEXT_CHALLENGE_REMOVED ? "уровня на сервере нет"
            : text == TEXT_CHALLENGE_UPDATED ? "уровень с тех пор обновлён"
                                             : "не скачать");
    std::lock_guard<std::mutex> lock(fetch->mu);
    fetch->state = state;
    fetch->text = text;
    fetch->localId = local;
}

}

extern "C" int medecomp_challenge_level_begin(const char* stat) {
    const std::string prefix = "challenge_u";
    std::string code(stat ? stat : "");
    if (code.compare(0, prefix.size(), prefix) != 0) return 0;
    std::string rest = code.substr(prefix.size());
    size_t v = rest.rfind('v');
    std::string serverId = rest.substr(0, v == std::string::npos ? 0 : v);
    int version = v == std::string::npos ? 0 : atoi(rest.c_str() + v + 1);
    bool plain = serverId.size() == 24 && version >= 1;
    for (char c : serverId) plain = plain && isxdigit((unsigned char)c);
    s_fetch = std::make_shared<ChallengeFetch>();
    if (!plain) {
        s_fetch->state = -1;
        return 1;
    }
    std::thread(fetchChallengeLevel, s_fetch, serverId, version).detach();
    return 1;
}

extern "C" int medecomp_challenge_level_poll(int* base, int* text) {
    if (!s_fetch) return *text = TEXT_CHALLENGE_FAILED, -1;
    std::string local;
    {
        std::lock_guard<std::mutex> lock(s_fetch->mu);
        if (s_fetch->state == 0) return 0;
        if (s_fetch->state < 0) {
            int state = s_fetch->state;
            *text = s_fetch->text;
            s_fetch.reset();
            return state;
        }
        local = s_fetch->localId;
    }
    s_fetch.reset();
    std::string error;
    bool ok = port::platform::preparePlayerLevel(local, &error);
    if (ok && s_bags) ok = false, error = "сбор сумок, а вызов — гонка";
    s_pending = false;
    s_solo = true;
    s_entered = false;
    s_base = ok ? port::platform::baseIndexOf(s_palette) : -1;
    if (ok && s_base < 0) ok = false, error = "главы " + s_palette + " нет в копии игры";
    if (!ok) {
        fprintf(stderr, "medecomp: вызов на уровне игрока %s: %s\n", local.c_str(), error.c_str());
        return *text = TEXT_CHALLENGE_FAILED, -1;
    }
    *base = s_base;
    fprintf(stderr, "medecomp: уровень игрока %s — вызов, на месте главы %d\n", local.c_str(), s_base);
    return 1;
}

extern "C" int medecomp_challenge_allowed(void) {
    return !playing() || !port::platform::playerLevelBoard().empty();
}
