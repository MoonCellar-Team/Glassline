#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class InputStream;
class String;

class Class : public Object {
public:
    Class();
    virtual ~Class();
    virtual Class* getClass() const;
    virtual String* getName() const = 0;
    virtual Object* newInstance() const = 0;
    static InputStream* getResourceAsStream(String* name);
};

}
