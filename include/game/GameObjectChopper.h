#pragma once

#include "game/GameObject.h"
#include "game/SignalFilter.h"
#include "game/Splines/SmoothSpline.h"

class Path;
namespace m3g {
class Node;
}

class GameObjectChopper : public GameObject {
public:
    static const int TYPE = 7;

    static const int UPDATE_DIST_SQ;

    GameObjectChopper(MEdgeMap* map, float x, float y, float z, int pathIndex);
    virtual ~GameObjectChopper();

    virtual void resetLevel();
    virtual void update(int dt);

    bool shouldUpdate();
    bool playerInRange();
    void shootPlayer();
    void updateSound(int dt);
    void updateOrientation(int dt);
    bool shouldShoot(int dt);
    void updateMovement(int dt);
    void setupSpline();
    void setModel();

    midp::ReferenceCountedPointer<m3g::Node> m_model;
    int m_unk80;
    m3g::Node* m_mainRotor;
    m3g::Node* m_tailRotor;
    m3g::Node* m_flashRight;
    m3g::Node* m_flashLeft;
    m3g::Node* m_unusedNodes[4];
    float m_rotorAngle;
    int m_distToPlayerSq;
    Path* m_path;
    int m_time;
    Splines::SmoothSpline m_spline;
    float m_splinePos;
    float m_splineLength;
    MathVector m_hover;
    float m_prevSpeed;
    float m_yaw;
    SignalFilter m_rollFilter;
    SignalFilter m_pitchFilter;
    SignalFilter m_speedFilter;
    int m_burstTime;
    int m_burstPause;
    int m_shotTimer;
    bool m_rotorFrame;
    int m_unk94c;
    int m_loopSounds[2];
};
