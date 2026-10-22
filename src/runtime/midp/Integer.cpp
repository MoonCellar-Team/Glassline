#include "runtime/midp/Integer.h"

#include "sdk/CoreFoundation.h"

namespace midp {

int Integer::parseInt(String* s) {
    return CFStringGetIntValue(s->getCFString());
}

}
