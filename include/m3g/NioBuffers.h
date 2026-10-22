#pragma once
#include "runtime/nio/Buffer.h"
#include "runtime/nio/ByteBuffer.h"
#include "runtime/nio/ByteOrder.h"
#include "runtime/nio/FloatBuffer.h"
#if 0
#include "runtime/midp/Object.h"
namespace nio {
class ByteOrder { public: static ByteOrder* nativeOrder(); };
class Buffer : public midp::Object {
public: int capacity() const; Buffer* limit(int value); Buffer* rewind();
};
class FloatBuffer : public Buffer {};
class ByteBuffer : public Buffer {
public: static ByteBuffer* allocateDirect(int bytes); ByteBuffer* order(ByteOrder* order);
};
}
#endif
