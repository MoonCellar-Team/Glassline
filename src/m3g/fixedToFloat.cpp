#include "m3g/fixedToFloat.h"

namespace m3g {

float fixedToFloat::operator()(int value) {
    return static_cast<float>(value) * 0.0000152587890625f;
}

}
