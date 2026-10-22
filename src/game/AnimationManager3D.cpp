#include "game/AnimationManager3D.h"

#include <stddef.h>

#include "game/MathExt.h"
#include "game/ResourceManager.h"
#include "runtime/midp/DataInputStream.h"

static const int RESOURCE_ANIMS_3D = 182;

static const int HERMITE_STEP = 0x40;
static const int HERMITE_ONE = 0x10000;
static const int HERMITE_ENTRIES = HERMITE_ONE / HERMITE_STEP + 1;

AnimationManager3D::AnimationManager3D() : m_hermiteTable(NULL) {
    m_hermiteTable = new int[HERMITE_ENTRIES * 4];
    int* entry = m_hermiteTable;
    for (int t = 0; t != HERMITE_ONE; t += HERMITE_STEP) {
        int t2 = MathExt::Fmul(t, t);
        int t3 = MathExt::Fmul(t2, t);
        entry[0] = 2 * t3 - 3 * t2 + HERMITE_ONE;
        entry[1] = 3 * t2 - 2 * t3;
        entry[2] = t3 - 2 * t2 + t;
        entry[3] = t3 - t2;
        entry += 4;
    }
}

int AnimationManager3D::getAnimationDuration(int anim) {
    int start = getAnimStartFrame(anim);
    int end = getAnimEndFrame(anim);
    if (end - start > 0 && start != 0 && end != 0) {
        return (end - start) * FRAME_TIME;
    }
    return 0;
}

bool AnimationManager3D::loadAnimFile(ResourceManager& resources) {
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_ANIMS_3D));
    int count = in.readShort();
    m_startFrames = midp::array<short>(count);
    m_endFrames = midp::array<short>(count);
    m_unk28 = midp::array<midp::array<short> >(count);
    m_unk3c = midp::array<midp::array<short> >(count);
    m_unk50 = midp::array<midp::array<signed char> >(count);
    for (int i = 0; i < count; i++) {
        m_startFrames[i] = in.readShort();
        m_endFrames[i] = in.readShort();
        in.readBoolean();
        int n = in.readShort();
        if (n <= 0) {
            continue;
        }
        m_unk28[i] = midp::array<short>(n);
        m_unk3c[i] = midp::array<short>(n);
        m_unk50[i] = midp::array<signed char>(n);
        for (int j = 0; j < n; j++) {
            m_unk28[i][j] = in.readShort();
            m_unk3c[i][j] = in.readShort();
            m_unk50[i][j] = in.readByte();
        }
    }
    return true;
}

AnimationManager3D::~AnimationManager3D() {
    if (m_hermiteTable != NULL) {
        delete[] m_hermiteTable;
        m_hermiteTable = NULL;
    }
}

int AnimationManager3D::getAnimStartFrame(int anim) {
    return m_startFrames[anim];
}

int AnimationManager3D::getAnimEndFrame(int anim) {
    return m_endFrames[anim];
}

int AnimationManager3D::getNumAnimations() {
    return m_startFrames.m_length;
}
