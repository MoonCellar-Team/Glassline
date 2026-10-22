#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class PlayerIPhoneOpenALClass : public midp::Class {
public:
    PlayerIPhoneOpenALClass();
    virtual ~PlayerIPhoneOpenALClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
