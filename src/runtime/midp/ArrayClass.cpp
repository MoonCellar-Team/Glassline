#include "runtime/midp/ArrayClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* ArrayClass::newInstance() const {
    return NULL;
}

ArrayClass::ArrayClass() {}

ArrayClass::~ArrayClass() {}

midp::String* ArrayClass::getName() const {
    return new midp::String("[");
}

}
