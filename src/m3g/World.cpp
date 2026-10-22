#include "m3g/World.h"
#include "m3g/Camera.h"
#include "m3g/Background.h"

namespace m3g {

World::World() : m_background(nullptr), m_activeCamera(nullptr) {}

World::~World() { midp::DECREF(m_background); midp::DECREF(m_activeCamera); m_background=nullptr; m_activeCamera=nullptr; }

void World::setActiveCamera(Camera* camera) { midp::INCREF(camera); midp::DECREF(m_activeCamera); m_activeCamera=camera; }

void World::setBackground(Background* background) { midp::INCREF(background); midp::DECREF(m_background); m_background=background; }

Camera* World::getActiveCamera() const { return m_activeCamera; }

Background* World::getBackground() const { return m_background; }

int World::getM3GUniqueClassID() const { return 22; }

}
