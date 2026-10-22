#pragma once

#include <string>
#include <vector>

#include "ea_online/Mayhem/Request.h"

namespace Mayhem {

class Stat;
class User;
class Token;

class GetLeaderboardRequest : public Request {
public:
    GetLeaderboardRequest(Token* token, const std::string& statId, int count);
    GetLeaderboardRequest(Token* token, const std::string& statId, const std::vector<long long>& userIds);

    int Size() const;
    User* GetUser(int index) const;
    Stat* GetStat(int index) const;
};

}
