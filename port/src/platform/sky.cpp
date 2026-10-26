#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <vector>

#include "m3g/Appearance.h"
#include "m3g/IndexBuffer.h"
#include "m3g/Mesh.h"
#include "m3g/Node.h"
#include "m3g/PolygonMode.h"
#include "m3g/Transform.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "m3g/m3g.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace {

const int kWindingCCW = 168, kWindingCW = 169;

midp::ReferenceCountedPointer<m3g::Node> s_dome(NULL);
midp::ReferenceCountedPointer<m3g::Mesh> s_copies[2] = {midp::ReferenceCountedPointer<m3g::Mesh>(NULL),
                                                        midp::ReferenceCountedPointer<m3g::Mesh>(NULL)};
m3g::Transform s_mirror[2];

bool lowEnd(m3g::Mesh* mesh, float* end) {
    m3g::VertexBuffer* vb = mesh->getVertexBuffer();
    float psb[4];
    m3g::VertexArray* positions = vb ? vb->getPositions(psb) : nullptr;
    if (!positions || positions->getComponentCount() != 3) return false;
    int n = positions->getVertexCount();
    std::vector<float> p(size_t(n) * 3);
    if (positions->getComponentType() == 1) {
        std::vector<signed char> v(p.size());
        positions->get(0, n, v.data());
        for (size_t i = 0; i < p.size(); i++) p[i] = v[i];
    } else if (positions->getComponentType() == 2) {
        std::vector<short> v(p.size());
        positions->get(0, n, v.data());
        for (size_t i = 0; i < p.size(); i++) p[i] = v[i];
    } else {
        positions->get(0, n, p.data());
    }
    std::vector<float> az(static_cast<size_t>(n)), el(static_cast<size_t>(n));
    for (int i = 0; i < n; i++) {
        float x = p[size_t(i) * 3] * psb[0] + psb[1], y = p[size_t(i) * 3 + 1] * psb[0] + psb[2],
              z = p[size_t(i) * 3 + 2] * psb[0] + psb[3];
        az[size_t(i)] = atan2f(fabsf(x), -z);
        el[size_t(i)] = atan2f(y, sqrtf(x * x + z * z));
    }
    float low = *std::min_element(el.begin(), el.end()), best = 0;
    for (int i = 0; i < n; i++)
        if (el[size_t(i)] <= low + 0.0524f) best = std::max(best, az[size_t(i)]);
    if (best < 0.6f || best > 1.6f) return false;
    *end = best;
    return true;
}

void mirrorAt(float a, m3g::Transform* out) {
    float ux = sinf(a), uz = -cosf(a);
    const float m[16] = {
        2 * ux * ux - 1, 0, 2 * ux * uz,     0,
        0,               1, 0,               0,
        2 * ux * uz,     0, 2 * uz * uz - 1, 0,
        0,               0, 0,               1,
    };
    out->set(m);
}

void build(m3g::Node* dome) {
    for (auto& copy : s_copies) {
        if (copy != NULL && copy->getParent()) static_cast<m3g::Group*>(copy->getParent())->removeChild(copy);
        copy = NULL;
    }
    s_dome = dome;
    m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(dome);
    m3g::Group* parent = static_cast<m3g::Group*>(dome->getParent());
    float end = 0;
    if (!mesh || !parent || mesh->getSubmeshCount() != 1 || !mesh->getAppearance(0) || !lowEnd(mesh, &end)) {
        fprintf(stderr, "medecomp: небо: купол не кусок кольца из одной сетки — копий по бокам нет (C45)\n");
        return;
    }
    m3g::Appearance* look = new m3g::Appearance();
    mesh->getAppearance(0)->duplicateTo(look);
    m3g::PolygonMode* mode = new m3g::PolygonMode();
    if (m3g::PolygonMode* was = mesh->getAppearance(0)->getPolygonMode()) was->duplicateTo(mode);
    mode->setWinding(mode->getWinding() == kWindingCW ? kWindingCCW : kWindingCW);
    look->setPolygonMode(mode);
    look->setLayer(std::max(-63, mesh->getAppearance(0)->getLayer() - 1));
    for (int side = 0; side < 2; side++) {
        s_copies[side] = new m3g::Mesh(mesh->getVertexBuffer(), mesh->getIndexBuffer(0), look);
        parent->addChild(s_copies[side]);
        mirrorAt(side ? -end : end, &s_mirror[side]);
    }
    fprintf(stderr, "medecomp: небо: по бокам купола — отражённые копии, кромка на ±%.1f° (C45)\n", end * 57.29578f);
}

}

extern "C" void medecomp_sky_follow(m3g::Node* dome) {
    if (dome == NULL) return;
    if (dome != s_dome) build(dome);
    for (int side = 0; side < 2; side++) {
        if (s_copies[side] == NULL) continue;
        m3g::Transform t;
        dome->getCompositeTransform(&t);
        t.postMultiply(&s_mirror[side]);
        s_copies[side]->setTransform(&t);
        s_copies[side]->setRenderingEnable(dome->isRenderingEnabled());
    }
}
