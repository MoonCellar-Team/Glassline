#pragma once
#include <map>
#include <string>
#include <vector>

namespace port {
namespace achievements {

const char* const kGame = "rooftops";

struct Entry {
    std::string key;
    std::string icon;
    std::map<std::string, std::string> title, description;
    int index = -1;
    std::string metric;
    int target = 0;
    std::string unit;

    const std::string& titleIn(bool russian) const;
    const std::string& descriptionIn(bool russian) const;
};

const std::vector<Entry>& catalog();
const Entry* byIndex(int index);
const Entry* byKey(const std::string& key);

struct Local {
    bool ok = false;
    std::vector<bool> unlocked;
    std::map<std::string, int> metrics;
};
Local readLocal();
int currentOf(const Entry& entry, const Local& local);

void queueUnlock(const std::string& key, long long unixTime);
void queueProgress(const std::string& key, int current);
void queueFromSave(const std::string& account);
int pending();

void flushAsync(const std::string& server, const std::string& token, const std::string& account);
bool flushing();
std::string lastError();

struct Remote {
    bool unlocked = false;
    std::string unlockedAt;
    int current = -1;
};
bool fetchMine(const std::string& server, const std::string& token, std::map<std::string, Remote>* out,
               std::string* error);

}
}

extern "C" bool medecomp_achievement_unlocked(int index);
extern "C" void medecomp_achievements_saved(void);
