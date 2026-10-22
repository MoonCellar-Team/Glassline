#include "runtime/midp/ByteArrayInputStreamClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* ByteArrayInputStreamClass::newInstance() const {
    return NULL;
}

ByteArrayInputStreamClass::ByteArrayInputStreamClass() {}

ByteArrayInputStreamClass::~ByteArrayInputStreamClass() {}

midp::String* ByteArrayInputStreamClass::getName() const {
    return new midp::String("java.io.ByteArrayInputStream");
}

}
