#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class ThreadClass : public midp::Class {
public:
    ThreadClass();
    virtual ~ThreadClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
