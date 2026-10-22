#include "runtime/midp/OutputStreamClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* OutputStreamClass::newInstance() const {
    return NULL;
}

OutputStreamClass::OutputStreamClass() {}

OutputStreamClass::~OutputStreamClass() {}

midp::String* OutputStreamClass::getName() const {
    return new midp::String("java.io.OutputStream");
}

}
