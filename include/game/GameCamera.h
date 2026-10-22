#pragma once

#include "game/AnimPlayer3D.h"
#include "game/MathVector.h"
#include "game/SignalFilter.h"
#include "m3g/Transform.h"
#include "runtime/midp/ReferenceCounted.h"

class DataCameraAnim;
class GameObject;
namespace m3g {
class Camera;
class Node;
}

class GameCamera : public midp::ReferenceCounted {
public:
    enum State {
        STATE_NONE = -1,
        STATE_SMOOTH_TRACKING = 1,
    };

    static const float STIFFNESS;
    static const float FOV_CUTSCENE;

    explicit GameCamera(bool splitScreen);
    virtual ~GameCamera();
    void setM3GCamera(m3g::Camera* camera);
    void loadM3GAnims();
    void update(int dt, float screenRotation);
    void startShake(float intensity, int duration);
    void startSmoothTracking(GameObject* target, bool);

    void setClipping(float nearClip, float farClip);
    bool isAnimating();
    void stateTransition(int state);
    void setLookFromLookAt(float fromX, float fromY, float fromZ, float atX, float atY, float atZ, bool immediate);
    static void createLookAtTransform(m3g::Transform* out, float fromX, float fromY, float fromZ, float atX,
                                      float atY, float atZ, float upX, float upY, float upZ);
    float getTargetSpeedFactor();
    void recalcCameraProjection();
    void recalcCameraTransform();
    void startCameraAnimation(int anim, bool immediate, int facingDir);
    void updateVerticalBump(int dt);
    void updateCameraProjection(int dt);
    void updateShaking(int dt);
    void updateDataAnim(int dt);
    void playDataAnim(DataCameraAnim* anim, GameObject* target);
    void updateCameraAnim(int dt);
    void updateFollowGameObject(int dt, bool immediate);
    const MathVector& getLookAtOffset();

    float getLookFromX();
    float getLookFromY();
    float getLookFromZ();
    float getLookAtX();
    float getLookAtY();
    float getLookAtZ();
    float getNearClip();
    float getFarClip();
    float getFOV();

    float m_nearClip;
    float m_farClip;
    MathVector m_lookFrom;
    MathVector m_lookFromTarget;
    MathVector m_lookAt;
    MathVector m_lookFromVelocity;
    MathVector m_lookAtOffset;
    MathVector m_cameraOffset;
    float m_velocityLead;
    float m_velocityLeadY;
    SignalFilter m_shakeX;
    SignalFilter m_shakeY;
    SignalFilter m_shakeZ;
    SignalFilter m_shakeAmount;
    SignalFilter m_lookAtX;
    SignalFilter m_lookAtY;
    SignalFilter m_lookAtZ;
    SignalFilter m_targetVelX;
    SignalFilter m_targetVelY;
    SignalFilter m_targetVelZ;
    SignalFilter m_targetSpeed;
    SignalFilter m_animBlend;
    SignalFilter m_verticalBump;
    SignalFilter m_fov;
    int m_state;
    int m_shakeTime;
    int m_bumpTime;
    int m_unk6c4;
    GameObject* m_target;

    union alignas(16) { m3g::Transform m_transform; char m_transformBytes[0x50]; };

    float m_aspectRatio;
    float m_fovScale;
    m3g::Camera* m_camera;
    int m_animFacingDir;
    union { AnimPlayer3D m_lookAtAnim; char m_lookAtAnimBytes[0x2c]; };
    union { AnimPlayer3D m_lookFromAnim; char m_lookFromAnimBytes[0x2c]; };
    m3g::Node* m_lookAtNode;
    m3g::Node* m_lookFromNode;
    bool m_dataAnimPlaying;
    DataCameraAnim* m_dataAnim;
    float m_lastTargetX;
};
