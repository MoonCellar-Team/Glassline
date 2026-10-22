#pragma once

#include "ea_online/Mayhem/Request.h"
#include "ea_online/Mayhem/User.h"

namespace Mayhem {

class UserRequest : public User, public Request {
public:
    using User::IsValid;
    virtual bool IsValid();
};

}
