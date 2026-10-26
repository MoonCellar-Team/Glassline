#pragma once

#include <string>
#include <utility>
#include <vector>

namespace port {
namespace online {

struct NetLevel {
    std::string id, name, description, base, author, board;
    std::string authorId, hash;
    std::string mode = "race";
    int timeLimit = 0;
    int version = 0, ratings = 0, downloads = 0;
    double rating = 0;
    long long updatedAt = 0;
    bool hidden = false;
};

bool listLevels(const std::string& sort, const std::string& q, int offset, int limit, std::vector<NetLevel>* out,
                int* total, std::string* error, const std::string& mode = std::string());
bool getLevel(const std::string& id, NetLevel* out, bool* missing, std::string* error);
bool downloadLevel(const NetLevel& level, std::string* localId, std::string* error);
bool uploadLevel(const std::string& localId, std::string* error);
bool uploadable(const std::string& localId);
bool rateLevel(const std::string& id, int stars, std::string* error, NetLevel* updated = nullptr);
bool deleteLevel(const std::string& serverId, std::string* error);
std::string uploadedId(const std::string& localId);
bool reportLevel(const std::string& id, const std::string& reason, std::string* error);
bool myLevels(std::vector<NetLevel>* out, std::string* authorId, std::string* error);
bool syncUploaded(int* changed, std::string* error);
enum class Online { Unknown, Same, Differs, Missing, OtherAccount };
struct OnlineState {
    Online state = Online::Unknown;
    bool newerThere = false;
    bool hidden = false;
};
OnlineState onlineState(const std::string& localId);
bool takeLevel(const std::string& localId, std::string* error);
bool levelLeaderboard(const std::string& board, int limit, std::vector<std::pair<std::string, int>>* out,
                      std::string* error);

}
}
