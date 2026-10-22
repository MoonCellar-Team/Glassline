#include "game/Path.h"

#include "game/GameCommon.h"
#include "game/PathPoint.h"
#include "runtime/midp/DataInputStream.h"

int Path::getPointCount() const {
    return m_points.m_length;
}

const PathPoint* Path::getPoint(int index) const {
    return m_points[index];
}

Path::~Path() {
    for (int i = 0; i < m_points.m_length; i++) {
        delete m_points[i];
    }
}

Path::Path(midp::DataInputStream& in) {
    int count = in.readShort();
    m_points = midp::array<PathPoint*>(count);
    for (int i = 0; i < count; i++) {
        m_points[i] = new PathPoint(in);
    }
}
