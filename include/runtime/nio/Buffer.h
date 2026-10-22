#pragma once
#include "runtime/midp/Object.h"

namespace midp {
class Class;
}

namespace nio {

class Buffer : public midp::Object {
public:
    explicit Buffer(int capacity);
    virtual ~Buffer();
    virtual midp::Class* getClass() const;
    virtual bool isReadOnly() const = 0;

    int capacity() const;
    int position() const;
    int limit() const;
    int remaining() const;
    Buffer* position(int value);
    Buffer* limit(int value);
    Buffer* rewind();
    void clearMark();
    void requireValidIndex(int index) const;

    int m_capacity;
    int m_limit;
    int m_position;
    int m_mark;
};

}
