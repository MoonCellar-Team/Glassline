#pragma once
#include "m3g/Node.h"
namespace m3g { class Camera; class Background; }

namespace m3g {

class World : public Group {
public:
    World();
    virtual ~World();
    void setActiveCamera(Camera* camera);
    void setBackground(Background* background);
    Camera* getActiveCamera() const;
    Background* getBackground() const;
    virtual int getM3GUniqueClassID() const;
    Background* m_background;
    Camera* m_activeCamera;
};

}
