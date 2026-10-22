#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class PolygonModeClass : public midp::Class {
public:
    PolygonModeClass();
    virtual ~PolygonModeClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
};

}
