#include "game/PathPoint.h"

#include "runtime/midp/DataInputStream.h"

PathPoint::PathPoint(midp::DataInputStream& in) : MathVector(), m_action(-1), m_speed(-1) {
    set(&in);
    m_action = in.readInt();
    m_speed = in.readInt();
}

PathPoint::~PathPoint() {}
