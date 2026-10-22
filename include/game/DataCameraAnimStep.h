#pragma once

#include "runtime/midp/ReferenceCounted.h"

namespace midp {
class DataInputStream;
}

class DataCameraAnimStep : public midp::ReferenceCounted {
public:
    DataCameraAnimStep(midp::DataInputStream& in, int startTime);
    virtual ~DataCameraAnimStep();

    int m_travelTime;
    int m_holdTime;
    int m_startTime;
    int m_arriveTime;
    int m_leaveTime;
    float m_lookFromX;
    float m_lookFromY;
    float m_lookFromZ;
    float m_lookAtX;
    float m_lookAtY;
    float m_lookAtZ;
};
