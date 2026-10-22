#include "runtime/midp/Object.h"
#include <stdint.h>

namespace midp {

Object::Object() {}

Object::~Object() {}

bool Object::equals(const Object* other) const {
    return this == other;
}

int Object::hashCode() const {
    return (int)(intptr_t)this;
}

}
