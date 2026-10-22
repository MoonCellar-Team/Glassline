#pragma once

#include "runtime/midp/array.h"

class QuadMesh;

class QuadAttrib {
public:
    QuadAttrib();
    QuadAttrib(const QuadAttrib& other);
    ~QuadAttrib();

    QuadMesh* m_mesh;
    int m_attrib;
    midp::array<int> m_values;
};
