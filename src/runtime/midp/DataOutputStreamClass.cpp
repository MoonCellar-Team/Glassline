#include "runtime/midp/DataOutputStreamClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* DataOutputStreamClass::newInstance() const {
    return NULL;
}

DataOutputStreamClass::DataOutputStreamClass() {}

DataOutputStreamClass::~DataOutputStreamClass() {}

midp::String* DataOutputStreamClass::getName() const {
    return new midp::String("java.io.DataOutputStream");
}

}
