#pragma once
#include "runtime/nio/ByteOrder.h"

namespace nio {

class LittleEndianByteOrder : public ByteOrder {
public:
    LittleEndianByteOrder();
    virtual ~LittleEndianByteOrder();
    virtual void putInt(signed char* dst, int value) const;
    virtual int getInt(const signed char* src) const;
};

}
