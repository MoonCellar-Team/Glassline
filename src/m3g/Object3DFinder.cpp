#include "m3g/Object3DFinder.h"
#include "m3g/Object3D.h"

namespace m3g {

Object3DFinder::Object3DFinder(int userID) : m_userID(userID), m_found(nullptr) {}

Object3D* Object3DFinder::getFound() const { return m_found; }

void Object3DFinder::find(Object3D* object) {
    if (object && !m_found) m_found = object->find(m_userID);
}

}
