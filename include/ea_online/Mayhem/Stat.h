#pragma once

#include <string>

namespace Mayhem {

class Stat {
public:
    virtual ~Stat();
    int GetValue() const;
    std::string GetStatID() const;
    int GetDecodedData(void** out) const;
};

}
