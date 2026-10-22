#include "game/ChunkRunnerVision.h"

#include "game/GameObjectRunner.h"
#include "game/M3GAssets.h"
#include "game/MathVector.h"
#include "m3g/Node.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Math.h"

static const float FIXED_TO_FLOAT = 1.0f / 65536.0f;

static const float COLOR_SPEED = 500.0f;

static float fadeByEdgeDistance(float d) {
    return midp::Math::min(1.0f, midp::Math::max(0.0f, (d + 15.0f) * 0.1f));
}

int ChunkRunnerVision::getType() {
    return m_type;
}

ChunkRunnerVision::ChunkRunnerVision(midp::DataInputStream& in, m3g::Node* node)
    : m_type(0), m_color(255), m_targetColor(255) {
    m_type = in.readByte();
    m_node = node;
    m_bounds.m_min.x = in.readInt() * FIXED_TO_FLOAT;
    m_bounds.m_min.y = in.readInt() * FIXED_TO_FLOAT;
    m_bounds.m_max.x = in.readInt() * FIXED_TO_FLOAT;
    m_bounds.m_max.y = in.readInt() * FIXED_TO_FLOAT;
}

ChunkRunnerVision::~ChunkRunnerVision() {}

void ChunkRunnerVision::updateIntensity(float dt, const MathVector& position, int facing) {
    float fadeX;
    if (facing == GameObjectRunner::FACING_RIGHT && m_type == TYPE_RIGHT) {
        if (position.x > m_bounds.m_max.x) {
            fadeX = 0.0f;
        } else {
            fadeX = fadeByEdgeDistance(position.x - m_bounds.m_min.x);
        }
    } else if (facing == GameObjectRunner::FACING_LEFT && m_type == TYPE_LEFT) {
        if (m_bounds.m_min.x > position.x) {
            fadeX = 0.0f;
        } else {
            fadeX = fadeByEdgeDistance(m_bounds.m_max.x - position.x);
        }
    } else if (m_type == TYPE_BOTH) {
        if (position.x <= m_bounds.m_min.x) {
            fadeX = fadeByEdgeDistance(position.x - m_bounds.m_min.x);
        } else if (position.x >= m_bounds.m_max.x) {
            fadeX = fadeByEdgeDistance(m_bounds.m_max.x - position.x);
        } else {
            fadeX = 1.0f;
        }
    } else {
        fadeX = 0.0f;
    }

    float dy;
    if (position.y < m_bounds.m_min.y) {
        dy = m_bounds.m_min.y - position.y;
    } else if (position.y > m_bounds.m_max.y) {
        dy = position.y - m_bounds.m_max.y;
    } else {
        dy = 0.0f;
    }
    float fadeY = midp::Math::min(1.0f, midp::Math::max(0.0f, (5.0f - dy) * 0.5f));

    m_targetColor = 255 - (int)(fadeY * fadeX * 255.0f);
    if (m_color == m_targetColor) {
        return;
    }
    if (m_color < m_targetColor) {
        m_color = midp::Math::min(midp::Math::max(1, (int)(dt * COLOR_SPEED)) + m_color, m_targetColor);
    } else if (m_targetColor < m_color) {
        m_color = midp::Math::max(m_color - midp::Math::max(1, (int)(dt * COLOR_SPEED)), m_targetColor);
    }
    M3GAssets::applyColor(m_node, (int)(0xffff0000 | (m_color << 8) | m_color));
}
