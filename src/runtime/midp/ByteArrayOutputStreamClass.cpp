#include "runtime/midp/ByteArrayOutputStreamClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* ByteArrayOutputStreamClass::newInstance() const {
    return NULL;
}

ByteArrayOutputStreamClass::ByteArrayOutputStreamClass() {}

ByteArrayOutputStreamClass::~ByteArrayOutputStreamClass() {}

midp::String* ByteArrayOutputStreamClass::getName() const {
    return new midp::String("java.io.ByteArrayOutputStream");
}

}
