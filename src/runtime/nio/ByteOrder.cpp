#include "runtime/nio/BigEndianByteOrder.h"
#include "runtime/nio/ByteOrder.h"
#include "runtime/nio/LittleEndianByteOrder.h"
#include "runtime/nio/NativeByteOrder.h"

namespace nio {

ByteOrder::ByteOrder() {}

ByteOrder::~ByteOrder() {}

namespace {

LittleEndianByteOrder s_little;
BigEndianByteOrder s_big;
NativeByteOrder s_native;

}

const ByteOrder* const ByteOrder::BIG_ENDIAN = &s_big;
const ByteOrder* const ByteOrder::NATIVE = &s_native;

const ByteOrder* ByteOrder::nativeOrder() {
    return NATIVE;
}

}
