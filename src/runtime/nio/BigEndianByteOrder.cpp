#include "runtime/nio/BigEndianByteOrder.h"

namespace nio {

BigEndianByteOrder::BigEndianByteOrder() {}

BigEndianByteOrder::~BigEndianByteOrder() {}

void BigEndianByteOrder::putInt(signed char* dst, int value) const {
    unsigned bits = static_cast<unsigned>(value);
    dst[0] = static_cast<signed char>(bits >> 24);
    dst[1] = static_cast<signed char>(bits >> 16);
    dst[2] = static_cast<signed char>(bits >> 8);
    dst[3] = static_cast<signed char>(bits);
}

int BigEndianByteOrder::getInt(const signed char* src) const {
    return static_cast<unsigned char>(src[3]);
}

}
