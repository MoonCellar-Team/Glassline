#pragma once

#include "game/Splines/TimedSpline.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class DataCameraAnimStep;
class GameCamera;
class GameObject;
namespace midp {
class DataInputStream;
}

class DataCameraAnim : public midp::ReferenceCounted {
public:
    explicit DataCameraAnim(midp::DataInputStream& in);
    virtual ~DataCameraAnim();
    void play(GameCamera* camera, GameObject* target);
    bool update(int dt);
    int getTargetID() const;

    GameCamera* m_camera;
    GameObject* m_target;
    midp::array<midp::ReferenceCountedPointer<DataCameraAnimStep> > m_steps;
    int m_targetID;
    bool m_playing;
    int m_splineTime;
    int m_time;
    int m_step;
    int m_duration;
    int m_unk3c;
    Splines::TimedSpline m_lookFromSpline;
    Splines::TimedSpline m_lookAtSpline;
};
