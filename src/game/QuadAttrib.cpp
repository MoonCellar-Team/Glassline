#include "game/QuadAttrib.h"

#include <stddef.h>

QuadAttrib::~QuadAttrib() {}

QuadAttrib::QuadAttrib() : m_mesh(NULL), m_attrib(0) {}

QuadAttrib::QuadAttrib(const QuadAttrib& other)
    : m_mesh(other.m_mesh), m_attrib(other.m_attrib), m_values(other.m_values) {}
