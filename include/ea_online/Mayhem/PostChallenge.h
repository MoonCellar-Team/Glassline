#pragma once

#include <string>

#include "ea_online/Mayhem/Request.h"
#include "runtime/midp/ReferenceCounted.h"

namespace Mayhem {

class Token;

class PostChallenge : public Request {
public:
    PostChallenge(Token* token, const std::string& mayhemId, const std::string& statTypeCode,
                  const std::string& userId, int value, const std::string& comment, int dataLength,
                  const void* data);
};

}
