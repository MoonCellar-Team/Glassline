#pragma once

namespace nio {

class ByteOrder {
public:
    virtual void putInt(signed char* dst, int value) const = 0;
    virtual int getInt(const signed char* src) const = 0;
    ByteOrder();
    virtual ~ByteOrder();

    static const ByteOrder* nativeOrder();
    static const ByteOrder* const BIG_ENDIAN;
    static const ByteOrder* const NATIVE;
};

}
