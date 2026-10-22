#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class Texture2DClass : public midp::Class {
public:
    Texture2DClass();
    virtual ~Texture2DClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
