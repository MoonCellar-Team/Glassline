#include "runtime/util/TimerTaskClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace util {

midp::Object* TimerTaskClass::newInstance() const {
    return NULL;
}

TimerTaskClass::TimerTaskClass() {}

TimerTaskClass::~TimerTaskClass() {}

midp::String* TimerTaskClass::getName() const {
    return new midp::String("java.util.TimerTask");
}

}
