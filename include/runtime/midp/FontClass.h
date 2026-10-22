#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class FontClass : public midp::Class {
public:
    FontClass();
    virtual ~FontClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
