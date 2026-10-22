#include "runtime/midp/RuntimeClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* RuntimeClass::newInstance() const {
    return NULL;
}

RuntimeClass::RuntimeClass() {}

RuntimeClass::~RuntimeClass() {}

midp::String* RuntimeClass::getName() const {
    return new midp::String("java.lang.Runtime");
}

}
