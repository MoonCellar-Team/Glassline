#include "runtime/midp/FileClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* FileClass::newInstance() const {
    return NULL;
}

FileClass::FileClass() {}

FileClass::~FileClass() {}

midp::String* FileClass::getName() const {
    return new midp::String("java.io.File");
}

}
