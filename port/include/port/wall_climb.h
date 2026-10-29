#pragma once

namespace port {

class WallClimbJump {
public:
    void start(float x) { m_remaining = 600; m_launchX = x; }
    void reset() { m_remaining = 0; }
    void update(int dt) { m_remaining = dt < m_remaining ? m_remaining - dt : 0; }
    bool allows(float x, float upVelocity) const {
        float moved = x - m_launchX;
        return m_remaining > 0 && upVelocity > -3.0f &&
               ((moved > 1.0f && moved <= 3.5f) || (moved < -1.0f && moved >= -3.5f));
    }

private:
    int m_remaining = 0;
    float m_launchX = 0.0f;
};

}
