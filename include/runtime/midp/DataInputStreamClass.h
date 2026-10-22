#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class DataInputStreamClass : public midp::Class {
public:
    DataInputStreamClass();
    virtual ~DataInputStreamClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
