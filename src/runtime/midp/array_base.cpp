#include "runtime/midp/array.h"

namespace midp {

class ArrayClass {
public:
    ArrayClass();
};

array_base::array_base(int length, array_data* data) : m_length(length), m_data(data) {
    INCREF(m_data);
}

array_base::array_base(const array_base& other) : Object(), m_length(other.m_length), m_data(other.m_data) {
    INCREF(m_data);
}

array_base::~array_base() {}

Class* array_base::getClass() const {
    return (Class*)new ArrayClass();
}

void* array_base::data() const {
    return m_data != NULL ? m_data->m_data : NULL;
}

void* array_base::discard() {
    array_data* d = m_data;
    void* p = data();
    bool owned = d != NULL && d->m_owned;
    m_length = 0;
    m_data = NULL;
    bool deleted = DECREF(d);
    return deleted && owned ? p : NULL;
}

void array_base::setLengthAndData(int length, array_data* data) {
    INCREF(data);
    discard();
    m_length = length;
    m_data = data;
}

array_base& array_base::operator=(const array_base& other) {
    setLengthAndData(other.m_length, other.m_data);
    return *this;
}

}
