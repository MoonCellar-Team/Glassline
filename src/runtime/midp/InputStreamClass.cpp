#include "runtime/midp/InputStreamClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* InputStreamClass::newInstance() const {
    return NULL;
}

InputStreamClass::InputStreamClass() {}

InputStreamClass::~InputStreamClass() {}

midp::String* InputStreamClass::getName() const {
    return new midp::String("java.io.InputStream");
}

}
