#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class Class;

class Control : public virtual Object {
public:
    Control();
    virtual ~Control();
    virtual Class* getClass() const;
};

}
