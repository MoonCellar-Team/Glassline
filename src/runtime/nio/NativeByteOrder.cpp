#include "runtime/nio/NativeByteOrder.h"

namespace nio {

NativeByteOrder::NativeByteOrder() {}

NativeByteOrder::~NativeByteOrder() {}

void NativeByteOrder::putInt(signed char* dst, int value) const {
    *reinterpret_cast<int*>(dst) = value;
}

int NativeByteOrder::getInt(const signed char* src) const {
    return *reinterpret_cast<const int*>(src);
}

}
