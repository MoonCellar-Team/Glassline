#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class AppearanceClass : public midp::Class {
public:
    AppearanceClass();
    virtual ~AppearanceClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
