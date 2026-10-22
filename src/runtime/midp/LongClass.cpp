#include "runtime/midp/LongClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* LongClass::newInstance() const {
    return NULL;
}

LongClass::LongClass() {}

LongClass::~LongClass() {}

midp::String* LongClass::getName() const {
    return new midp::String("java.lang.Long");
}

}
