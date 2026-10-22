#include "game/PainBox.h"

#include <stddef.h>

#include "game/CollShape.h"
#include "game/GameObject.h"
#include "runtime/midp/DataInputStream.h"

bool PainBox::intersects(const GameObject& obj) {
    return CollShape::testIntersection(m_shape, obj.getShape());
}

PainBox::~PainBox() {
    midp::DECREF(m_shape);
}

PainBox::PainBox(midp::DataInputStream& in) : m_shape(NULL) {
    float minX = in.readFloat();
    float minY = in.readFloat();
    float minZ = in.readFloat();
    float maxX = in.readFloat();
    float maxY = in.readFloat();
    float maxZ = in.readFloat();
    m_shape = new CollOrthoHexahedron(minX, minY, minZ, maxX, maxY, maxZ);
    midp::INCREF(m_shape);
}
