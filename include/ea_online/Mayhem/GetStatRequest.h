#pragma once

#include <string>

#include "ea_online/Mayhem/Request.h"
#include "ea_online/Mayhem/Stat.h"

namespace Mayhem {

class GetStatRequest : public Stat, public Request {
public:
    GetStatRequest(const std::string& statId, bool withData);
};

}
