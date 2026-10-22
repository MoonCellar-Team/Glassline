#include "runtime/util/RandomClass.h"

#include <stddef.h>
#include "runtime/util/JRandom.h"
#include "runtime/midp/String.h"

namespace util {

midp::Object* RandomClass::newInstance() const {
    return new util::JRandom();
}

RandomClass::RandomClass() {}

RandomClass::~RandomClass() {}

midp::String* RandomClass::getName() const {
    return new midp::String("java.util.Random");
}

}
