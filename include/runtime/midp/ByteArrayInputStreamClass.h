#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class ByteArrayInputStreamClass : public midp::Class {
public:
    ByteArrayInputStreamClass();
    virtual ~ByteArrayInputStreamClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
