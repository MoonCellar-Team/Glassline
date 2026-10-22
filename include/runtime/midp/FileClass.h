#pragma once

#include "runtime/midp/Class.h"

namespace midp {

class FileClass : public midp::Class {
public:
    FileClass();
    virtual ~FileClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
