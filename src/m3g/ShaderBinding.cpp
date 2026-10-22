#include "m3g/ShaderBinding.h"

namespace m3g {

ShaderBinding::ShaderBinding(int flags) : m_program(0), m_flags(flags), m_textureBindings(nullptr), m_skinBindings(nullptr) {
    int count = 0;
    while (count < 4 && ((flags >> (16 + count * 3)) & 7) && ((flags >> (4 + count * 2)) & 3)) ++count;
    m_textureBindings = new unsigned char[count * 12];
    if (flags & 4) m_skinBindings = new unsigned char[0x50];
}

ShaderBinding::~ShaderBinding() { delete[] m_textureBindings; delete[] m_skinBindings; }

}
