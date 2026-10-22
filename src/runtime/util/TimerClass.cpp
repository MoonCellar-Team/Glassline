#include "runtime/util/TimerClass.h"

#include <stddef.h>
#include "runtime/util/Timer.h"
#include "runtime/midp/String.h"

namespace util {

midp::Object* TimerClass::newInstance() const {
    return new util::Timer();
}

TimerClass::TimerClass() {}

TimerClass::~TimerClass() {}

midp::String* TimerClass::getName() const {
    return new midp::String("java.util.Timer");
}

}
