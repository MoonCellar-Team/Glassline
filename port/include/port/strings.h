#pragma once

#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

namespace midp {
class String;
}

namespace port {

void applyStringOverrides(int pool, midp::array<midp::ReferenceCountedPointer<midp::String>>& strings,
                          midp::String* locale);

midp::String* withInputHints(int textId, midp::String* raw, midp::String* locale);

}
