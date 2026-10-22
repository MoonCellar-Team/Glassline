#pragma once
#include "m3g/Transform.h"

namespace m3g {

class TransformStack {
public:
    TransformStack();
    explicit TransformStack(int capacity);
    ~TransformStack();
    Transform* top() const;
    void pop();
    void push();
    void translate(float x,float y,float z) const;
    void scale(float x,float y,float z) const;
    void loadAndClear(const Transform* transform);
    void loadIdentity() const;
    void setCapacity(int capacity);
    int m_capacity;
    int m_top;
    Transform* m_transforms;
};

}
