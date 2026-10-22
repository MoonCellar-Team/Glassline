#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class PlayerClass : public midp::Class {
public:
    PlayerClass();
    virtual ~PlayerClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
