#include "game/DataCameraAnim.h"

#include <stddef.h>

#include "game/DataCameraAnimStep.h"
#include "game/GameCamera.h"
#include "game/GameObject.h"
#include "game/MathVector.h"
#include "runtime/midp/DataInputStream.h"

int DataCameraAnim::getTargetID() const {
    return m_targetID;
}

DataCameraAnim::~DataCameraAnim() {}

bool DataCameraAnim::update(int dt) {
    if (!m_playing) {
        return false;
    }
    MathVector lookFrom(m_camera->getLookFromX(), m_camera->getLookFromY(), m_camera->getLookFromZ());
    MathVector lookAt(m_camera->getLookAtX(), m_camera->getLookAtY(), m_camera->getLookAtZ());
    int time = m_time;
    int endTime = m_time + dt;
    int left = dt;
    do {
        DataCameraAnimStep* step = m_steps[m_step];
        int slice = left;
        if (endTime > step->m_leaveTime) {
            slice = step->m_leaveTime - time;
        }
        time += slice;
        int travel = slice;
        if (time > step->m_arriveTime) {
            travel = slice - (time - step->m_arriveTime);
        }
        if (travel >= 1) {
            m_splineTime += travel;
            lookFrom = m_lookFromSpline.getPosition(m_splineTime);
            if (m_target == NULL) {
                lookAt = m_lookAtSpline.getPosition(m_splineTime);
            }
        } else if (step->m_startTime == step->m_arriveTime) {
            lookFrom = m_lookFromSpline.getNodePosition(m_step + 1);
            if (m_target == NULL) {
                lookAt = m_lookAtSpline.getNodePosition(m_step + 1);
            }
        }
        left -= slice;
    } while (left > 0 && ++m_step < m_steps.m_length);
    m_time = endTime;
    if (endTime > m_duration) {
        m_playing = false;
    }
    if (m_target != NULL) {
        lookAt.set(m_target->m_position.x, m_target->m_position.y, m_target->m_position.z);
        lookAt += m_camera->getLookAtOffset();
    }
    m_camera->setLookFromLookAt(lookFrom.x, lookFrom.y, lookFrom.z, lookAt.x, lookAt.y, lookAt.z, true);
    return m_playing;
}

void DataCameraAnim::play(GameCamera* camera, GameObject* target) {
    m_target = target;
    m_camera = camera;
    m_splineTime = 0;
    m_time = 0;
    m_step = 0;
    m_playing = true;
    m_lookFromSpline.clear();
    m_lookAtSpline.clear();
    MathVector lookFrom(m_camera->getLookFromX(), m_camera->getLookFromY(), m_camera->getLookFromZ());
    m_lookFromSpline.addNode(lookFrom, 0);
    MathVector lookAt(m_camera->getLookAtX(), m_camera->getLookAtY(), m_camera->getLookAtZ());
    m_lookAtSpline.addNode(lookAt, 0);
    for (int i = 0; i < m_steps.m_length; i++) {
        MathVector stepFrom(m_steps[i]->m_lookFromX, m_steps[i]->m_lookFromY, m_steps[i]->m_lookFromZ);
        MathVector stepAt(m_steps[i]->m_lookAtX, m_steps[i]->m_lookAtY, m_steps[i]->m_lookAtZ);
        m_lookFromSpline.addNode(stepFrom, m_steps[i]->m_travelTime);
        m_lookAtSpline.addNode(stepAt, m_steps[i]->m_travelTime);
    }
    m_lookFromSpline.buildSpline();
    m_lookAtSpline.buildSpline();
}

DataCameraAnim::DataCameraAnim(midp::DataInputStream& in)
    : m_camera(NULL),
      m_target(NULL),
      m_targetID(0),
      m_playing(false),
      m_splineTime(0),
      m_time(0),
      m_step(0),
      m_duration(0),
      m_unk3c(0) {
    m_targetID = in.readByte();
    int count = in.readShort();
    m_steps = midp::array<midp::ReferenceCountedPointer<DataCameraAnimStep> >(count);
    m_duration = 0;
    for (int i = 0; i < count; i++) {
        m_steps[i] = new DataCameraAnimStep(in, m_duration);
        m_duration = m_steps[i]->m_travelTime + m_duration + m_steps[i]->m_holdTime;
    }
}
