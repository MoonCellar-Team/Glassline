#include "m3g/TransformStack.h"
#include "m3g/Placement.h"
void* operator new[](__SIZE_TYPE__);
void operator delete[](void*) noexcept;

namespace m3g {

TransformStack::TransformStack() : m_capacity(0),m_top(0),m_transforms(nullptr) {}

TransformStack::TransformStack(int capacity) : m_capacity(0),m_top(0),m_transforms(nullptr) { setCapacity(capacity); }

TransformStack::~TransformStack() {
    delete[] m_transforms;
}

Transform* TransformStack::top() const { return m_transforms+m_top; }

void TransformStack::pop() { --m_top; }

void TransformStack::push() { m_transforms[m_top+1].set(top()); ++m_top; }

void TransformStack::translate(float x,float y,float z) const { top()->postTranslate(x,y,z); }

void TransformStack::scale(float x,float y,float z) const { top()->postScale(x,y,z); }

void TransformStack::loadAndClear(const Transform* transform) { m_top=0; m_transforms->set(transform); }

void TransformStack::loadIdentity() const { top()->setIdentity(); }

void TransformStack::setCapacity(int capacity) {
    m_capacity=capacity;
    Transform* transforms=new Transform[capacity];
    m_transforms=transforms;
}

}
