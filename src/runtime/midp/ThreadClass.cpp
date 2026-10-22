#include "runtime/midp/ThreadClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* ThreadClass::newInstance() const {
    return NULL;
}

ThreadClass::ThreadClass() {}

ThreadClass::~ThreadClass() {}

midp::String* ThreadClass::getName() const {
    return new midp::String("java.util.Thread");
}

}
