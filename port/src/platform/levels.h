#pragma once

#include <stdint.h>

#include <string>
#include <vector>

namespace midp {
class StringBuffer;
}

namespace port {
namespace online {
struct Json;
}
namespace platform {

constexpr int kBagTimeLimits[3] = {120000, 180000, 300000};
constexpr int kBagTimeLimitDefault = 180000;
struct LevelMode {
    bool bags = false;
    int timeLimit = 0;
};
inline bool arenaBase(const std::string& palette) { return palette.compare(0, 12, "multiplayer_") == 0 && palette.find("chapter") == std::string::npos; }
LevelMode readLevelMode(const std::string& id);
bool supportsVisible(const port::online::Json& info);
std::string bestBags(const std::string& id);

bool preparePlayerLevel(const std::string& id, std::string* error);
std::string checkPlayerMap(const std::vector<unsigned char>& data, bool bags = false);
void playerLevelFrame();
void setTestRun(bool on);
bool testRun();

bool startPlayerLevelTogether(const std::string& id, int* base, std::string* error, bool solo = false);
void clearPlayerLevel();
struct SharedLevel {
    std::string name, author, palette, serverId;
    bool bags = false;
    int timeLimit = 0, serverVersion = 0;
    uint64_t hash = 0;
    std::vector<unsigned char> map;
};
bool currentSharedLevel(SharedLevel* out);
uint64_t levelHash(const std::vector<unsigned char>& data);
std::string findLevelByMap(uint64_t hash, size_t size);
std::string savePeerLevel(const SharedLevel& level, std::string* error);
struct MultiplayerLevel {
    std::string id, name;
    bool bags = false;
    int base = -1;
};
std::vector<MultiplayerLevel> multiplayerLevels();
bool playerLevelPlaying();
std::string playerLevelBoard();
std::string playingBestBags();

bool soloBagsChoosing();
midp::StringBuffer* playerBestLine(const std::string& id);

}
}
