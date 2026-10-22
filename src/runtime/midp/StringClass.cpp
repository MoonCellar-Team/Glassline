#include "runtime/midp/StringClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* StringClass::newInstance() const {
    return new midp::String();
}

StringClass::StringClass() {}

StringClass::~StringClass() {}

midp::String* StringClass::getName() const {
    return new midp::String("java.lang.String");
}

}
