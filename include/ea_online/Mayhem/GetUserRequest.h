#pragma once

#include "ea_online/Mayhem/UserRequest.h"

namespace Mayhem {

class GetUserRequest : public UserRequest {
public:
    explicit GetUserRequest(bool);
    GetUserRequest(long long facebookId, bool);
};

}
