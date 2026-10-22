#pragma once
#include "runtime/midp/ReferenceCounted.h"

namespace midp {

class Object : public ReferenceCounted {
public:
    Object();
    virtual ~Object();
    virtual bool equals(const Object* other) const;
    virtual int hashCode() const;
};

}
