#include "game/Scene.h"

#include "game/SceneClass.h"

midp::Class* Scene::getClass() const {
    return new SceneClass();
}

Scene::~Scene() {}

Scene::Scene(AppEngine* engine) : m_engine(engine), m_loadingProgress(0) {}
