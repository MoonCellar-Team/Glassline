#include "runtime/nio/LittleEndianByteOrder.h"

namespace nio {

LittleEndianByteOrder::LittleEndianByteOrder() {}

LittleEndianByteOrder::~LittleEndianByteOrder() {}

void LittleEndianByteOrder::putInt(signed char* dst, int value) const {
    unsigned bits = static_cast<unsigned>(value);
    dst[0] = static_cast<signed char>(bits);
    dst[1] = static_cast<signed char>(bits >> 8);
    dst[2] = static_cast<signed char>(bits >> 16);
    dst[3] = static_cast<signed char>(bits >> 24);
}

int LittleEndianByteOrder::getInt(const signed char* src) const {
    return static_cast<unsigned char>(src[0]);
}

}
