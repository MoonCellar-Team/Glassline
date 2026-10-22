#pragma once
#include "runtime/nio/ByteOrder.h"

namespace nio {

class BigEndianByteOrder : public ByteOrder {
public:
    BigEndianByteOrder();
    virtual ~BigEndianByteOrder();
    virtual void putInt(signed char* dst, int value) const;
    virtual int getInt(const signed char* src) const;
};

}
