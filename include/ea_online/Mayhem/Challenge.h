#pragma once

#include <string>

#include "ea_online/Mayhem/Request.h"

namespace Mayhem {

class Token;

class Challenge {
public:
    int GetDecodedData(void** out) const;
    const std::string& GetStatTypeCode() const;
    int GetValue() const;
};

class GetChallenge : public Challenge, public Request {
public:
    GetChallenge(Token* token, const std::string& mayhemId, const std::string& challengeId);
};

}
