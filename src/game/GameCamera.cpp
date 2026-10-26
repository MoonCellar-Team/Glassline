#include "game/GameCamera.h"

#include <math.h>

#include "game/AppEngine.h"
#include "game/DataCameraAnim.h"
#include "game/GameCommon.h"
#include "game/GameObject.h"
#include "game/M3GAssets.h"
#include "game/ResourceManager.h"
#include "game/SceneGame.h"
#include "m3g/Camera.h"
#include "runtime/midp/Math.h"

static const int RESOURCE_CAMERA_ANIMS = 0;
static const int NODE_ID_LOOK_AT = 175;
static const int NODE_ID_LOOK_FROM = 176;

static const float FOV_DEFAULT = 55.0f;
static const float FOV_SPEED_BONUS = 20.0f;

static const float DAMPING = 12.0f;

static const int CAMERA_ANIM_FLAGS = 16;

const float GameCamera::STIFFNESS = 25.0f;

const float GameCamera::FOV_CUTSCENE = 55.0f;

static bool sameBits(float a, float b) {
    union {
        float f;
        int i;
    } ua, ub;
    ua.f = a;
    ub.f = b;
    return ua.i == ub.i;
}

#ifdef MEDECOMP_PORT
static float s_shakeShare = 1.0f;
#endif

void GameCamera::setClipping(float nearClip, float farClip) {
    m_nearClip = nearClip;
    m_farClip = farClip;
}

bool GameCamera::isAnimating() {
    return m_lookFromAnim.isAnimating() || m_lookAtAnim.isAnimating() || m_dataAnimPlaying;
}

void GameCamera::stateTransition(int state) {
    m_state = state;
}

void GameCamera::startShake(float intensity, int duration) {
    m_shakeAmount.setTargetValue(intensity);
    m_shakeTime = duration;
}

void GameCamera::setLookFromLookAt(float fromX, float fromY, float fromZ, float atX, float atY, float atZ,
                                   bool immediate) {
    if (immediate) {
        m_lookFrom.x = fromX;
        m_lookFrom.y = fromY;
        m_lookFrom.z = fromZ;
        m_lookAtX.setSteadyState(atX);
        m_lookAtY.setSteadyState(atY);
        m_lookAtZ.setSteadyState(atZ);
    } else {
        m_lookFromTarget.x = fromX;
        m_lookFromTarget.y = fromY;
        m_lookFromTarget.z = fromZ;
        m_lookAtX.setTargetValue(atX);
        m_lookAtY.setTargetValue(atY);
        m_lookAtZ.setTargetValue(atZ);
    }
}

void GameCamera::createLookAtTransform(m3g::Transform* out, float fromX, float fromY, float fromZ, float atX,
                                       float atY, float atZ, float upX, float upY, float upZ) {
    float zx = fromX - atX;
    float zy = fromY - atY;
    float zz = fromZ - atZ;
    float invLen = 1.0f / sqrtf(zx * zx + zy * zy + zz * zz);
    zx *= invLen;
    zy *= invLen;
    zz *= invLen;

    float upError = midp::Math::abs(upX * upX + upY * upY + upZ * upZ - 1.0f);
    float xx = zz * upY - zy * upZ;
    float xy = zx * upZ - zz * upX;
    float xz = zy * upX - zx * upY;
    if (upError >= 1e-7f) {
        float invX = 1.0f / sqrtf(xx * xx + xy * xy + xz * xz);
        xx *= invX;
        xy *= invX;
        xz *= invX;
    }

    float m[16] = {
        xx, xy, xz, 0.0f,
        zy * xz - zz * xy, zz * xx - zx * xz, zx * xy - zy * xx, 0.0f,
        zx, zy, zz, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    out->set(m);
    out->postTranslate(-fromX, -fromY, -fromZ);
    out->invert();
}

float GameCamera::getTargetSpeedFactor() {
    return m_targetSpeed.getFilteredValue();
}

void GameCamera::recalcCameraProjection() {
    float blend = m_animBlend.getFilteredValue();
    float nearClip = m_nearClip;
    MathVector lookAt(m_lookAtX.getFilteredValue(), m_lookAtY.getFilteredValue(), m_lookAtZ.getFilteredValue());
    float distSq = (m_lookFrom - lookAt).getLengthSq();
    if (distSq < 81.0f) {
        float limit = sqrt((double)distSq) * 0.3f;
        if (limit < nearClip) {
            nearClip = limit;
        }
    }
    float fov = m_fov.getFilteredValue();
#ifdef MEDECOMP_PORT
    if (m_fovScale == 1.0f) {
        m_camera->setPerspective(medecomp_wide_fov((FOV_CUTSCENE - fov) * blend + fov), m_aspectRatio, nearClip,
                                 m_farClip);
        return;
    }
#endif
#ifdef MEDECOMP_SPLIT_STACKED
    m_camera->setPerspective(medecomp_hor_fov(m_fovScale * ((FOV_CUTSCENE - fov) * blend + fov), 1.0f, m_aspectRatio),
                             m_aspectRatio, nearClip, m_farClip);
    return;
#endif
    m_camera->setPerspective(m_fovScale * ((FOV_CUTSCENE - fov) * blend + fov), m_aspectRatio, nearClip,
                             m_farClip);
}

void GameCamera::setM3GCamera(m3g::Camera* camera) {
    m_camera = camera;
    recalcCameraProjection();
}

void GameCamera::recalcCameraTransform() {
    float shake = m_shakeAmount.getFilteredValue();
    float shakeX = m_shakeX.getFilteredValue() * shake;
    float shakeY = m_shakeY.getFilteredValue() * shake;
    float shakeZ = m_shakeZ.getFilteredValue() * shake;
    float bump = m_verticalBump.getFilteredValue();

#ifdef MEDECOMP_PORT
    float fromX = m_lookFrom.x + shakeX * s_shakeShare;
    float fromY = m_lookFrom.y + (shakeY + bump) * s_shakeShare;
    float fromZ = m_lookFrom.z + shakeZ * s_shakeShare;
#else
    float fromX = m_lookFrom.x + shakeX;
    float fromY = m_lookFrom.y + shakeY + bump;
    float fromZ = m_lookFrom.z + shakeZ;
#endif
    float atX = m_lookAtX.getFilteredValue();
    float atY = m_lookAtY.getFilteredValue();
    float atZ = m_lookAtZ.getFilteredValue();
    float upX = shakeX * 0.2f;
    float upY = shakeY * 0.2f + 1.0f;
    float upZ = shakeZ * 0.2f;

    float t[3];
    m_lookFromNode->getTranslation(t);
    float animFromX = m_animFacingDir * t[2] + m_target->m_position.x;
    float animFromY = t[1] + m_target->m_position.y;
    float animFromZ = m_target->m_position.z - t[0];
    m_lookAtNode->getTranslation(t);
    float animAtX = m_animFacingDir * t[2] + m_target->m_position.x;
    float animAtY = t[1] + m_target->m_position.y;
    float animAtZ = m_target->m_position.z - t[0];

    float blend = m_animBlend.getFilteredValue();
    m_lookFrom.x = (animFromX - fromX) * blend + fromX;
    m_lookFrom.y = (animFromY - fromY) * blend + fromY;
    m_lookFrom.z = (animFromZ - fromZ) * blend + fromZ;
    m_lookAt.x = (animAtX - atX) * blend + atX;
    m_lookAt.y = (animAtY - atY) * blend + atY;
    m_lookAt.z = (animAtZ - atZ) * blend + atZ;
    createLookAtTransform(&m_transform, m_lookFrom.x, m_lookFrom.y, m_lookFrom.z, m_lookAt.x, m_lookAt.y,
                          m_lookAt.z, upX - upX * blend, (1.0f - upY) * blend + upY, upZ - upZ * blend);
}

void GameCamera::startCameraAnimation(int anim, bool immediate, int facingDir) {
    if (immediate) {
        m_animBlend.setSteadyState(1.0f);
    } else {
        m_animBlend.setTargetValue(1.0f);
    }
    m_animFacingDir = facingDir;
    m_lookAtAnim.startAnim(anim, CAMERA_ANIM_FLAGS);
    m_lookFromAnim.startAnim(anim, CAMERA_ANIM_FLAGS);
    recalcCameraTransform();
}

void GameCamera::updateVerticalBump(int dt) {
    m_bumpTime -= dt;
    if (m_bumpTime < 0) {
        m_verticalBump.setTargetValue(0.0f);
    }
    m_verticalBump.update(dt);
}

void GameCamera::updateCameraProjection(int dt) {
    float speed = midp::Math::abs(getTargetSpeedFactor());
    m_fov.setTargetValue(speed * FOV_SPEED_BONUS + FOV_DEFAULT);
    m_fov.update(dt);
    recalcCameraProjection();
}

void GameCamera::updateShaking(int dt) {
    m_shakeTime -= dt;
    if (m_shakeTime > 0) {
        m_shakeX.setTargetValue((100 - AppEngine::getCanvas()->rand(0, 200)) * 0.01f);
        m_shakeY.setTargetValue((100 - AppEngine::getCanvas()->rand(0, 200)) * 0.01f);
        m_shakeZ.setTargetValue((100 - AppEngine::getCanvas()->rand(0, 200)) * 0.01f);
    } else {
        m_shakeX.setTargetValue(0.0f);
        m_shakeY.setTargetValue(0.0f);
        m_shakeZ.setTargetValue(0.0f);
    }
    m_shakeX.update(dt);
    m_shakeY.update(dt);
    m_shakeZ.update(dt);
    m_shakeAmount.update(dt);
}

void GameCamera::updateDataAnim(int dt) {
    m_targetSpeed.setTargetValue(0.0f);
    m_targetSpeed.update(dt);
    if (!m_dataAnim->update(dt)) {
        m_dataAnimPlaying = false;
        m_dataAnim = NULL;
    }
}

void GameCamera::playDataAnim(DataCameraAnim* anim, GameObject* target) {
    m_dataAnimPlaying = true;
    m_dataAnim = anim;
    anim->play(this, target);
}

void GameCamera::updateCameraAnim(int dt) {
    if (m_dataAnimPlaying) {
        updateDataAnim(dt);
        return;
    }
    if (isAnimating()) {
        m_lookAtAnim.updateAnim(dt);
        m_lookFromAnim.updateAnim(dt);
    }
    if (!isAnimating()) {
        m_animBlend.setTargetValue(0.0f);
    }
}

void GameCamera::updateFollowGameObject(int dt, bool immediate) {
    float speed;
    if (sameBits(m_target->m_position.x, m_lastTargetX)) {
        speed = 0.0f;
    } else {
        speed = midp::Math::max(-6.0f, midp::Math::min(m_target->getForwardVelocity(), 6.0f)) * (1.0f / 6.0f);
    }
    m_targetSpeed.setTargetValue(speed);
    m_targetSpeed.update(dt);

    float timeFactor = AppEngine::getCanvas()->getSceneGame()->getGameTimeFactor();
    GameObject* target = m_target;
    const MathVector& velocity = target->getVelocity();
    m_targetVelX.setTargetValue(velocity.x);
    m_targetVelY.setTargetValue(velocity.y);
    m_targetVelZ.setTargetValue(velocity.z);
    m_targetVelX.update(dt);
    m_targetVelY.update(dt);
    m_targetVelZ.update(dt);

    float leadX = m_targetVelX.getFilteredValue() * m_velocityLead * timeFactor;
    float leadY = m_targetVelY.getFilteredValue() * m_velocityLeadY * timeFactor;
    float leadZ = m_targetVelZ.getFilteredValue() * m_velocityLead * timeFactor;
    MathVector lookAt(target->m_position.x + leadX + m_lookAtOffset.x,
                      target->m_position.y + leadY + m_lookAtOffset.y,
                      target->m_position.z + leadZ + m_lookAtOffset.z);
    if (immediate) {
        m_lookAtX.setSteadyState(lookAt.x + m_cameraOffset.x);
        m_lookAtY.setSteadyState(lookAt.y + m_cameraOffset.y);
        m_lookAtZ.setSteadyState(lookAt.z + m_cameraOffset.z * timeFactor);
    } else {
        m_lookAtX.setTargetValue(lookAt.x);
        m_lookAtY.setTargetValue(lookAt.y);
        m_lookAtZ.setTargetValue(lookAt.z);
    }

    m_lookFromTarget.x = m_lookAtX.getFilteredValue() + m_cameraOffset.x;
    m_lookFromTarget.y = m_lookAtY.getFilteredValue() + m_cameraOffset.y;
    m_lookFromTarget.z = m_lookAtZ.getFilteredValue() + m_cameraOffset.z * timeFactor;
    if (immediate) {
        m_lookFrom = m_lookFromTarget;
        m_lookFromVelocity.set(0.0f, 0.0f, 0.0f);
    }
}

void GameCamera::startSmoothTracking(GameObject* target, bool immediate) {
    if (immediate) {
        m_animBlend.setSteadyState(0.0f);
    } else {
        m_animBlend.setTargetValue(0.0f);
    }
    m_target = target;
    updateFollowGameObject(0, immediate);
    recalcCameraTransform();
    stateTransition(STATE_SMOOTH_TRACKING);
}

void GameCamera::update(int dt, float screenRotation) {
    updateCameraProjection(dt);
    updateShaking(dt);
    updateVerticalBump(dt);
    if (!m_dataAnimPlaying) {
        updateFollowGameObject(dt, false);
    }
    updateCameraAnim(dt);
    m_lookAtX.update(dt);
    m_lookAtY.update(dt);
    m_lookAtZ.update(dt);
    m_animBlend.update(dt);

    if (!isAnimating()) {
        MathVector pos(m_lookFrom);
        MathVector spring = (pos - m_lookFromTarget) * -STIFFNESS;
        MathVector damping = m_lookFromVelocity * DAMPING;
        MathVector force = spring - damping;
        float t = dt * 0.001f;
        MathVector dv = force * 1.0f * t;
        m_lookFromVelocity += dv;
        m_lookFrom = pos + m_lookFromVelocity * t;
    }

#ifdef MEDECOMP_PORT
    s_shakeShare = dt < 33 ? dt / 34.0f : 1.0f;
    recalcCameraTransform();
    s_shakeShare = 1.0f;
#else
    recalcCameraTransform();
#endif
    m_transform.postRotate(screenRotation, 0.0f, 0.0f, 1.0f);
    m_camera->setTransform(&m_transform);
    m_lastTargetX = m_target->m_position.x;
}

void GameCamera::loadM3GAnims() {
    m3g::Object3D* root = AppEngine::getCanvas()->getResourceManager().loadM3GNode(RESOURCE_CAMERA_ANIMS);
    m3g::Object3D* fromObj = root->find(NODE_ID_LOOK_FROM);
    m3g::Object3D* atObj = root->find(NODE_ID_LOOK_AT);
    m_lookFromNode = m3g::m3g_cast<m3g::Node>(fromObj);
    m_lookAtNode = m3g::m3g_cast<m3g::Node>(atObj);
    midp::INCREF(m_lookAtNode);
    midp::INCREF(m_lookFromNode);
    M3GAssets::orphanNode(m_lookFromNode);
    M3GAssets::orphanNode(m_lookAtNode);
    midp::DECREF(root);
    m_lookAtAnim.setNode(m_lookAtNode);
    m_lookFromAnim.setNode(m_lookFromNode);
}

GameCamera::~GameCamera() {
    midp::DECREF(m_lookAtNode);
    m_lookAtNode = NULL;
    midp::DECREF(m_lookFromNode);
    m_lookFromNode = NULL;
    m_lookFromAnim.~AnimPlayer3D();
    m_lookAtAnim.~AnimPlayer3D();
    m_transform.~Transform();
}

GameCamera::GameCamera(bool splitScreen)
    : m_nearClip(0.0f),
      m_farClip(1.0f),
      m_lookFrom(0.0f, 0.0f, 1.0f),
      m_lookFromTarget(0.0f, 0.0f, 1.0f),
      m_lookAt(0.0f, 0.0f, 0.0f),
      m_lookFromVelocity(0.0f, 0.0f, 0.0f),
      m_lookAtOffset(),
      m_cameraOffset(),
      m_velocityLead(0.0f),
      m_velocityLeadY(0.0f),
      m_shakeX(SignalFilter::SMOOTHSTEP_FIR, 350.0f, 0.0f),
      m_shakeY(SignalFilter::SMOOTHSTEP_FIR, 350.0f, 0.0f),
      m_shakeZ(SignalFilter::SMOOTHSTEP_FIR, 350.0f, 0.0f),
      m_shakeAmount(SignalFilter::ONE_POLE_LP, 400.0f, 0.0f),
      m_lookAtX(SignalFilter::ONE_POLE_LP, 400.0f, 0.0f),
      m_lookAtY(SignalFilter::ONE_POLE_LP, 400.0f, 0.0f),
      m_lookAtZ(SignalFilter::ONE_POLE_LP, 400.0f, 0.0f),
      m_targetVelX(SignalFilter::ONE_POLE_LP, 2000.0f, 0.0f),
      m_targetVelY(SignalFilter::ONE_POLE_LP, 2000.0f, 0.0f),
      m_targetVelZ(SignalFilter::ONE_POLE_LP, 2000.0f, 0.0f),
      m_targetSpeed(SignalFilter::ONE_POLE_LP, 1900.0f, 0.0f),
      m_animBlend(SignalFilter::SMOOTHSTEP_FIR, 1200.0f, 0.0f),
      m_verticalBump(SignalFilter::ONE_POLE_LP, 1500.0f, 0.0f),
      m_fov(SignalFilter::ONE_POLE_LP, 500.0f, FOV_DEFAULT),
      m_state(STATE_NONE),
      m_shakeTime(0),
      m_bumpTime(0),
      m_unk6c4(0),
      m_target(NULL),
      m_transform(),
      m_aspectRatio(0.0f),
      m_fovScale(1.0f),
      m_camera(NULL),
      m_animFacingDir(1),
      m_lookAtAnim(AppEngine::getCanvas()->getAnimationManager3D()),
      m_lookFromAnim(AppEngine::getCanvas()->getAnimationManager3D()),
      m_lookAtNode(NULL),
      m_lookFromNode(NULL),
      m_dataAnimPlaying(false),
      m_dataAnim(NULL),
      m_lastTargetX(0.0f) {
    int width = AppEngine::getCanvas()->getWidth();
    float height = AppEngine::getCanvas()->getHeight();
    if (splitScreen) {
        m_aspectRatio = height / ((width - 10) * 0.5f);
#ifdef MEDECOMP_SPLIT_STACKED
        m_aspectRatio = medecomp_wide_aspect((height - 10) * 0.5f);
#endif
        m_fovScale = 0.5f;
        m_lookAtOffset = CAMERA_TARGET_OFFSET_SPLIT;
        m_cameraOffset = CAMERA_OFFSET_SPLIT;
        m_velocityLead = 0.5f;
    } else {
        m_aspectRatio = height / width;
#ifdef MEDECOMP_PORT
        m_aspectRatio = medecomp_wide_aspect(height);
#endif
        m_lookAtOffset = CAMERA_TARGET_OFFSET;
        m_cameraOffset = CAMERA_OFFSET;
        m_velocityLead = 0.4f;
    }
    m_velocityLeadY = 0.0f;
}

const MathVector& GameCamera::getLookAtOffset() {
    return m_lookAtOffset;
}

float GameCamera::getLookAtX() {
    return m_lookAt.x;
}

float GameCamera::getLookAtY() {
    return m_lookAt.y;
}

float GameCamera::getLookAtZ() {
    return m_lookAt.z;
}

float GameCamera::getLookFromX() {
    return m_lookFrom.x;
}

float GameCamera::getLookFromY() {
    return m_lookFrom.y;
}

float GameCamera::getLookFromZ() {
    return m_lookFrom.z;
}

float GameCamera::getFarClip() {
    return m_farClip;
}

float GameCamera::getNearClip() {
    return m_nearClip;
}

float GameCamera::getFOV() {
    return m_fov.getFilteredValue() + m_animBlend.getFilteredValue() * (FOV_CUTSCENE - m_fov.getFilteredValue());
}
