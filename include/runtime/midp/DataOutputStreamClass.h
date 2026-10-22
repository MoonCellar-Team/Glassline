#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class DataOutputStreamClass : public midp::Class {
public:
    DataOutputStreamClass();
    virtual ~DataOutputStreamClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
