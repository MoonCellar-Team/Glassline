#pragma once
#include "runtime/midp/Class.h"

namespace m3g {

class GroupClass : public midp::Class {
public:
    GroupClass();
    virtual ~GroupClass();
    virtual midp::String* getName() const;
    virtual midp::Object* newInstance() const;
    virtual bool _decRef();
    virtual void _incRef();
};

}
