#pragma once

#include "ea_online/Mayhem/User.h"

namespace Mayhem {

class UserList {
public:
    virtual ~UserList();
    int Size() const;
    User* GetMayhemUser(int index);
};

}
