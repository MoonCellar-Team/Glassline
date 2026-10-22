#include "runtime/midp/ReferenceCountedPointer.h"

namespace util {
class JRandom;
}

template midp::ReferenceCountedPointer<util::JRandom>::operator util::JRandom*() const;
