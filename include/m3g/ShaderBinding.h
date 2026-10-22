#pragma once

namespace m3g {

class ShaderBinding {
public:
    explicit ShaderBinding(int flags);
    ~ShaderBinding();
    int& location(unsigned index) { return reinterpret_cast<int*>(m_unk8)[index]; }
    int location(unsigned index) const { return reinterpret_cast<const int*>(m_unk8)[index]; }
    int* texture(unsigned unit) const { return reinterpret_cast<int*>(m_textureBindings+unit*12); }
    int* palette() const { return reinterpret_cast<int*>(m_skinBindings); }
    int m_program;
    int m_flags;
    unsigned char m_unk8[24];
    unsigned char* m_textureBindings;
    unsigned char* m_skinBindings;
};

}
