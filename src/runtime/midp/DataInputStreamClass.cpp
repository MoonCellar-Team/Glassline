#include "runtime/midp/DataInputStreamClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* DataInputStreamClass::newInstance() const {
    return NULL;
}

DataInputStreamClass::DataInputStreamClass() {}

DataInputStreamClass::~DataInputStreamClass() {}

midp::String* DataInputStreamClass::getName() const {
    return new midp::String("java.io.DataInputStream");
}

}
