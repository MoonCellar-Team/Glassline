#pragma once

#include <vector>

#include "ea_online/Mayhem/Request.h"
#include "ea_online/Mayhem/UserList.h"

namespace Mayhem {

class GetUserListRequest : public UserList, public Request {
public:
    explicit GetUserListRequest(const std::vector<long long>& ids);
};

}
