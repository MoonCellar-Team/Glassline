#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class ByteArrayOutputStreamClass : public midp::Class {
public:
    ByteArrayOutputStreamClass();
    virtual ~ByteArrayOutputStreamClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
