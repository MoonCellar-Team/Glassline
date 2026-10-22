#pragma once

#include <string>

namespace Mayhem {

class User {
public:
    virtual ~User();
    virtual const std::string& GetMayhemID() const;
    virtual bool IsValid() const;
    const std::string& GetDisplayName() const;
};

}
