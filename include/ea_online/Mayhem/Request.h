#pragma once

#include <string>

#include "runtime/midp/ReferenceCounted.h"

namespace Mayhem {

class Request : public midp::ReferenceCounted {
public:
    enum Result {
        RESULT_PENDING = 0,
        RESULT_SUCCESS = 1,
    };

    int GetResult();
    const std::string& GetErrorType() const;
};

}
