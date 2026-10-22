#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class SkinnedMeshClass : public midp::Class {
public:
    SkinnedMeshClass();
    virtual ~SkinnedMeshClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
