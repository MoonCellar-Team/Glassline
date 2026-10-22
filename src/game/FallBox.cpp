#include "game/FallBox.h"

#include <stddef.h>

#include "game/CollShape.h"
#include "runtime/midp/DataInputStream.h"

FallBox::~FallBox() {
    midp::DECREF(m_shape);
}

FallBox::FallBox(midp::DataInputStream& in) : m_shape(NULL) {
    float minX = in.readFloat();
    float minY = in.readFloat();
    float minZ = in.readFloat();
    float maxX = in.readFloat();
    float maxY = in.readFloat();
    float maxZ = in.readFloat();
    m_shape = new CollOrthoHexahedron(minX, minY, minZ, maxX, maxY, maxZ);
    midp::INCREF(m_shape);
}

bool FallBox::contains(const MathVector& point) {
    return m_shape->pointIntersects(point);
}
