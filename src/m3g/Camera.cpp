#include "m3g/Camera.h"
#include "runtime/midp/Math.h"

namespace m3g {

Camera::Camera() : m_projectionType(0x30),m_fovy(0),m_aspect(0),m_nearPlane(0),m_farPlane(0),m_projection(),m_projectionValid(true) {}

Camera::~Camera() { m_projectionType=0; m_fovy=0; m_aspect=0; m_nearPlane=0; m_farPlane=0; m_projectionValid=false; }

int Camera::getM3GUniqueClassID() const { return 5; }

void Camera::setParallel(float fovy,float aspect,float nearPlane,float farPlane) { m_farPlane=farPlane; m_fovy=fovy; m_aspect=aspect; m_nearPlane=nearPlane; m_projectionType=0x31; m_projectionValid=false; }

void Camera::setPerspective(float fovy,float aspect,float nearPlane,float farPlane) { m_farPlane=farPlane; m_fovy=fovy; m_aspect=aspect; m_nearPlane=nearPlane; m_projectionType=0x32; m_projectionValid=false; }

void Camera::setGeneric(const Transform* transform) { m_projectionType=0x30; m_projection.set(transform); m_projectionValid=true; }

int Camera::getProjection(Transform* out) const { if(out) { updateCachedProjectionTransform(); out->set(&m_projection); } return m_projectionType; }

void Camera::updateCachedProjectionTransform() const {
    if(m_projectionType==0x32) {
        float tangent=midp::Math::tan(m_fovy*0.008726646192371845f);
        float yScale=1.0f/tangent;
        float xScale=1.0f/(m_aspect*tangent);
        float zScale=-(m_nearPlane+m_farPlane)/(m_farPlane-m_nearPlane);
        float zOffset=((m_nearPlane*m_farPlane)*-2.0f)/(m_farPlane-m_nearPlane);
        float matrix[16]={0}; matrix[0]=xScale; matrix[5]=yScale; matrix[10]=zScale; matrix[11]=zOffset; matrix[14]=-1.0f;
        m_projection.set(matrix);
    } else if(m_projectionType==0x31) {
        float width=m_aspect*m_fovy;
        float depth=m_farPlane-m_nearPlane;
        float matrix[16]={0}; matrix[0]=2.0f/width; matrix[5]=2.0f/m_fovy;
        matrix[10]=-2.0f/depth; matrix[11]=-(m_nearPlane+m_farPlane)/depth; matrix[15]=1.0f;
        m_projection.set(matrix);
    }
    m_projectionValid=true;
}

}
