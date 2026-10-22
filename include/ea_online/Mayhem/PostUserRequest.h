#pragma once

#include <string>

#include "ea_online/Mayhem/UserRequest.h"

namespace Mayhem {

class PostUserRequest : public UserRequest {
public:
    explicit PostUserRequest(const std::string& name);
};

}
