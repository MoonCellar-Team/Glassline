#pragma once
#include "runtime/nio/ByteOrder.h"

namespace nio {

class NativeByteOrder : public ByteOrder {
public:
    NativeByteOrder();
    virtual ~NativeByteOrder();
    virtual void putInt(signed char* dst, int value) const;
    virtual int getInt(const signed char* src) const;
};

}
