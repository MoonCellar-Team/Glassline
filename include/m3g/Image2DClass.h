#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class Image2DClass : public midp::Class {
public:
    Image2DClass();
    virtual ~Image2DClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
