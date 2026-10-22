#include "runtime/midp/Long.h"

#include "runtime/midp/LongClass.h"

namespace midp {

Class* Long::getClass() const {
    return new LongClass();
}

Long::~Long() {}

Long::Long(long long value) : m_value(value) {}

}
