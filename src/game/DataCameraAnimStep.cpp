#include "game/DataCameraAnimStep.h"

#include "runtime/midp/DataInputStream.h"

DataCameraAnimStep::DataCameraAnimStep(midp::DataInputStream& in, int startTime)
    : m_travelTime(0),
      m_holdTime(0),
      m_startTime(0),
      m_arriveTime(0),
      m_leaveTime(0),
      m_lookFromX(0),
      m_lookFromY(0),
      m_lookFromZ(0),
      m_lookAtX(0),
      m_lookAtY(0),
      m_lookAtZ(0) {
    m_travelTime = in.readInt();
    m_holdTime = in.readInt();
    m_lookFromX = in.readFloat();
    m_lookFromY = in.readFloat();
    m_lookFromZ = in.readFloat();
    m_lookAtX = in.readFloat();
    m_lookAtY = in.readFloat();
    m_lookAtZ = in.readFloat();
    m_startTime = startTime;
    m_arriveTime = m_travelTime + startTime;
    m_leaveTime = m_travelTime + startTime + m_holdTime;
}

DataCameraAnimStep::~DataCameraAnimStep() {}
