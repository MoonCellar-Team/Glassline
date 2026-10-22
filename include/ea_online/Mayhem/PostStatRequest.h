#pragma once

#include <string>

#include "ea_online/Mayhem/Request.h"
#include "ea_online/Mayhem/Stat.h"

namespace Mayhem {

class Token;

class PostStatRequest : public Stat, public Request {
public:
    PostStatRequest(Token* token, const std::string& mayhemId, const std::string& statTypeCode, int value,
                    int dataLength, const void* data);
};

}
