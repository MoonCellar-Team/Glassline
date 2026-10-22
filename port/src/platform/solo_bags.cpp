#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "levels.h"
#include "port/cf.h"
#include "port/files.h"
#include "port/net.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

extern "C" int medecomp_player_level_solo(void);
extern "C" void medecomp_player_level_bags(int collected, int total);
extern "C" int medecomp_player_level_playing(void);

namespace {

const int kTextBest = 0x9ce;

struct Best {
    int collected = -1, total = 0, ms = 0;
};

bool s_menu;
std::string s_arena;
int s_limit;

std::string path() { return port::files::dataDir() + "/solo_bags.txt"; }

std::string uidOf(const Level* level) {
    midp::String* uid = level ? level->getUniqueName() : nullptr;
    return uid ? port::cf::utf8(uid->getCFString()) : std::string();
}

std::map<std::pair<std::string, int>, Best> load() {
    std::map<std::pair<std::string, int>, Best> out;
    std::vector<unsigned char> text;
    if (!port::files::read(path(), &text)) return out;
    std::istringstream in(std::string(text.begin(), text.end()));
    std::string uid;
    int limit;
    Best b;
    while (in >> uid >> limit >> b.collected >> b.total >> b.ms) out[{uid, limit}] = b;
    return out;
}

bool better(const Best& a, const Best& b) {
    if (a.collected != b.collected) return a.collected > b.collected;
    return a.collected == a.total && a.ms < b.ms;
}

bool arenaRun() {
    LevelData* levels = AppEngine::getLevelData();
    return s_menu && !s_arena.empty() && !port::net::rivalsNetwork() &&
           levels->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED &&
           uidOf(levels->getCurrentLevelObject()) == s_arena;
}

}

namespace port {
namespace platform {

bool soloBagsChoosing() { return s_menu; }

}
}

extern "C" void medecomp_solo_bags_menu(int on) {
    s_menu = on != 0;
    if (!s_menu) s_arena.clear();
}

extern "C" int medecomp_solo_bags_choosing(void) { return s_menu; }

extern "C" void medecomp_solo_bags_chosen(const Level* level, int timeLimit) {
    if (medecomp_player_level_playing()) {
        s_arena.clear();
        return;
    }
    s_arena = uidOf(level);
    s_limit = timeLimit;
    fprintf(stderr, "medecomp: сбор сумок в одиночку — %s, %d с\n", s_arena.c_str(), timeLimit / 1000);
}

extern "C" int medecomp_solo_bags(void) { return medecomp_player_level_solo() || arenaRun(); }

extern "C" void medecomp_solo_bags_result(int collected, int total, int timeLeft) {
    if (medecomp_player_level_solo()) {
        medecomp_player_level_bags(collected, total);
        return;
    }
    if (!arenaRun()) return;
    Best now;
    now.collected = collected;
    now.total = total;
    now.ms = collected == total ? std::max(0, s_limit - std::max(0, timeLeft)) : 0;
    auto all = load();
    auto key = std::make_pair(s_arena, s_limit);
    auto it = all.find(key);
    bool record = it == all.end() || better(now, it->second);
    fprintf(stderr, "medecomp: сбор сумок %s, %d с: %d из %d%s\n", s_arena.c_str(), s_limit / 1000, collected, total,
            record ? " — лучший сбор" : "");
    if (!record) return;
    all[key] = now;
    std::string text;
    for (const auto& e : all)
        text += e.first.first + " " + std::to_string(e.first.second) + " " + std::to_string(e.second.collected) + " " +
                std::to_string(e.second.total) + " " + std::to_string(e.second.ms) + "\n";
    if (FILE* f = fopen(path().c_str(), "w")) {
        fwrite(text.data(), 1, text.size(), f);
        fclose(f);
    }
}

namespace {

Best fromBestBags(const std::string& text) {
    Best b;
    if (sscanf(text.c_str(), "%d/%d", &b.collected, &b.total) != 2) b = Best();
    return b;
}

midp::StringBuffer* bestLine(const Best& b) {
    AppEngine* app = AppEngine::getCanvas();
    midp::StringBuffer* line = app->getStatLabel(kTextBest);
    line->append(' ');
    app->appendStatValue(line, AppEngine::STAT_OF, b.collected < 0 ? -1 : (b.collected << 16) | b.total);
    if (b.collected >= 0 && b.ms > 0) {
        line->append(' ');
        app->appendStatValue(line, AppEngine::STAT_TIME, b.ms);
    }
    return line;
}

}

namespace port {
namespace platform {

midp::StringBuffer* playerBestLine(const std::string& id) { return bestLine(fromBestBags(bestBags(id))); }

}
}

extern "C" midp::StringBuffer* medecomp_solo_bags_best_text(const Level* level, int timeLimit) {
    Best b;
    if (!level && medecomp_player_level_solo()) {
        b = fromBestBags(port::platform::playingBestBags());
    } else {
        if (!level && arenaRun()) {
            level = AppEngine::getLevelData()->getCurrentLevelObject();
            timeLimit = s_limit;
        }
        auto all = load();
        auto it = level ? all.find({uidOf(level), timeLimit}) : all.end();
        if (it != all.end()) b = it->second;
    }
    return bestLine(b);
}
