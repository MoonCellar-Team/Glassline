#pragma once

#include <string>

#include "ea_online/Mayhem/Request.h"

namespace Mayhem {

class Token : public Request {
public:
    Token(const std::string& userId, const std::string&);
    bool IsValid() const;
    void Refresh();
};

}
