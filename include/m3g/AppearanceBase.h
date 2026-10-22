#pragma once
#include "m3g/Object3D.h"
namespace m3g { class LineMode; class PolygonMode; class CompositingMode; }

namespace m3g {

class AppearanceBase : public Object3D {
public:
    AppearanceBase();
    virtual ~AppearanceBase();
    void setLayer(int layer);
    int getLayer() const;
    void setCompositingMode(CompositingMode* mode);
    void setPolygonMode(PolygonMode* mode);
    void setLineMode(LineMode* mode);
    CompositingMode* getCompositingMode() const;
    PolygonMode* getPolygonMode() const;
    LineMode* getLineMode() const;
    virtual int getReferences(midp::array<Object3D*>& references) const;
    virtual void duplicateTo(Object3D* target);
    int m_layer;
    LineMode* m_lineMode;
    PolygonMode* m_polygonMode;
    CompositingMode* m_compositingMode;
};

}
