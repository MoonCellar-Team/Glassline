#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class Graphics3DClass : public midp::Class {
public:
    Graphics3DClass();
    virtual ~Graphics3DClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
