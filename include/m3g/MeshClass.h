#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class MeshClass : public midp::Class {
public:
    MeshClass();
    virtual ~MeshClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
