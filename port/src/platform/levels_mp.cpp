#include <stdio.h>

#include <map>
#include <string>

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/UI/ChapterArrowedScroller.h"
#include "game/UI/ChapterSelectItem.h"
#include "game/UI/ChapterSelectItemMultiplayer.h"
#include "levels.h"
#include "port/cf.h"
#include "runtime/midp/String.h"

namespace {

class PlayerLevelItem : public UI::ChapterSelectItemMultiplayer {
public:
    PlayerLevelItem(const Level& base, const std::string& name) : ChapterSelectItemMultiplayer(base) {
        midp::ReferenceCountedPointer<midp::String> text(
            new midp::String(static_cast<const __CFString*>(port::cf::makeString(name))));
        m_name = text->toUpperCase();
    }
};

std::map<const UI::WindowElement*, std::string> s_items, s_names;
const UI::WindowElement* s_shown;

}

extern "C" bool medecomp_mp_add_player_levels(UI::ChapterArrowedScroller* race, UI::ChapterArrowedScroller* timed) {
    s_items.clear();
    s_names.clear();
    s_shown = nullptr;
    LevelData* levels = AppEngine::getLevelData();
    bool anyRace = false;
    for (const port::platform::MultiplayerLevel& l : port::platform::multiplayerLevels()) {
        UI::ChapterArrowedScroller* list = l.bags ? timed : race;
        if (l.base < 0 || !list) continue;
        PlayerLevelItem* item = new PlayerLevelItem(*levels->getLevel(l.base), l.name);
        s_items[item] = l.id;
        s_names[item] = l.name;
        list->addItem(item);
        anyRace = anyRace || !l.bags;
    }
    return anyRace;
}

extern "C" const char* medecomp_mp_item_name(const UI::WindowElement* item) {
    auto it = s_names.find(item);
    return it == s_names.end() ? nullptr : it->second.c_str();
}

extern "C" void medecomp_mp_choose(const UI::WindowElement* item) {
    auto it = s_items.find(item);
    if (it == s_items.end()) {
        port::platform::clearPlayerLevel();
        return;
    }
    int base = -1;
    std::string error;
    if (!port::platform::startPlayerLevelTogether(it->second, &base, &error, port::platform::soloBagsChoosing())) {
        fprintf(stderr, "medecomp: уровень игрока %s вдвоём: %s\n", it->second.c_str(), error.c_str());
        port::platform::clearPlayerLevel();
    }
}

extern "C" int medecomp_mp_item_new_limit(const UI::WindowElement* item) {
    if (item == s_shown) return 0;
    s_shown = item;
    auto it = s_items.find(item);
    return it == s_items.end() ? 0 : port::platform::readLevelMode(it->second).timeLimit;
}

extern "C" midp::StringBuffer* medecomp_solo_bags_best_text(const Level* level, int timeLimit);

extern "C" midp::StringBuffer* medecomp_solo_bags_item_best(const UI::WindowElement* item, int timeLimit) {
    auto it = s_items.find(item);
    if (it != s_items.end()) return port::platform::playerBestLine(it->second);
    const UI::ChapterSelectItem* chapter = static_cast<const UI::ChapterSelectItem*>(item);
    return medecomp_solo_bags_best_text(chapter ? chapter->getLevelObject() : nullptr, timeLimit);
}
