#include "runtime/midp/array.h"

namespace midp {

array_data::array_data(void* data, int length, int elementSize, bool owned)
    : m_data(data), m_length(length), m_elementSize(elementSize), m_owned(owned) {}

array_data::~array_data() {}

}
