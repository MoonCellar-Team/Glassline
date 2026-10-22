#pragma once
#include "runtime/midp/Class.h"

namespace midp { template <class T> class array; }

namespace m3g {

class AppearanceBase;
class Background;
class IndexBuffer;
class Transform;
class VertexArray;
class VertexBuffer;

class Renderer : public midp::Object {
public:
    Renderer();
    virtual ~Renderer();
    virtual midp::Class* getClass() const;
    virtual void setProjectionAndViewTransform(const Transform* projection,const Transform* view) = 0;
    virtual void pushModelTransform(const Transform* transform) = 0;
    virtual void popModelTransform() = 0;
    virtual void setViewport(int x,int y,int width,int height) = 0;
    virtual void clear(const Background* background) = 0;
    virtual void bind(int width,int height) = 0;
    virtual void release() = 0;
    virtual void render(const VertexBuffer* vertices,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha) = 0;
    virtual void render(const VertexBuffer* vertices,const VertexArray* skinIndices,const VertexArray* skinWeights,
                        const midp::array<Transform*>& palette,const IndexBuffer* indices,const AppearanceBase* appearance,
                        float alpha) = 0;
};

}
