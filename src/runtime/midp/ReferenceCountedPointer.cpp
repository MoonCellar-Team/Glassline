#include "runtime/midp/ReferenceCountedPointer.h"

#include "game/Scene.h"
#include "game/SceneGame.h"
#include "game/SceneMenu.h"
#include "game/SceneStartup.h"
#include "runtime/midp/Displayable.h"
#include "runtime/midp/Player.h"
#include "runtime/midp/Thread.h"

class AnimPlayer3D;
class AnimationBlender;
class ChunkColumn;
class ChunkLayer;
class ChunkRunnerVision;
class ChunkSection;
class CollOctahedron;
class CollOrthoHexahedron;
class CollShape;
class DataCameraAnim;
class DataCameraAnimStep;
class GameCamera;
class GameObject;
class GameObjectCollectable;
class GameObjectCulledGraphic;
class GameObjectGhost;
class GameObjectPlayer;
class GameScreen;
class GhostAnimationPlayback;
class ReflectionMapShader;
class SceneGame;
class SceneMenu;
class SceneStartup;
namespace GameUI { class Confirmation; class EndOfLevelResults; class ScrollingText; class SoundSettings; }
namespace m3g { class AnimationController; class Appearance; class AppearanceBase; class Camera; class Group; class Image2D; class KeyframeSequence; class Node; class Texture2D; class Transform; class World; }
namespace particles { class CylinderParticles; class EmissionMode; class ParticleEffect; class ParticleMode; class Particles; class PointParticles; class QuadParticles; }
namespace util { class JRandom; }
namespace midp {
class Displayable;
class File;
class Graphics;
class Image;
class ImageIPhone;
class InputStream;
class OutputStream;
class Player;
class SharedALBuffer;
class String;
class StringBuffer;
class Thread;
class WebImage;
}

namespace {

template <class T>
midp::ReferenceCounted* refVBase(T* p) {
    return p;
}

}

namespace midp {

template <>
ReferenceCountedPointer<Image>::ReferenceCountedPointer(Image* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<Image>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<String>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<Scene>::~ReferenceCountedPointer() {
    DECREF(refVBase(m_ptr));
}

template <>
ReferenceCountedPointer<Thread>::~ReferenceCountedPointer() {
    DECREF(refVBase(m_ptr));
}

template <>
ReferenceCountedPointer<StringBuffer>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::Transform>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::Node>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::Appearance>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::Texture2D>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<ReflectionMapShader>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<Scene>::ReferenceCountedPointer(Scene* p) : m_ptr(p) {
    INCREF(refVBase(p));
}

template <>
ReferenceCountedPointer<Scene>&
ReferenceCountedPointer<Scene>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(refVBase(other.m_ptr));
    DECREF(refVBase(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<Thread>::ReferenceCountedPointer(Thread* p) : m_ptr(p) {
    INCREF(refVBase(p));
}

template <>
ReferenceCountedPointer<Thread>&
ReferenceCountedPointer<Thread>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(refVBase(other.m_ptr));
    DECREF(refVBase(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<Thread>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<StringBuffer>::ReferenceCountedPointer(StringBuffer* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<m3g::Transform>::ReferenceCountedPointer(m3g::Transform* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<m3g::Node>::ReferenceCountedPointer(m3g::Node* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<m3g::Appearance>::ReferenceCountedPointer(m3g::Appearance* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<m3g::Texture2D>::ReferenceCountedPointer(m3g::Texture2D* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<ReflectionMapShader>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
template <>
ReferenceCountedPointer<ReflectionMapShader>&
ReferenceCountedPointer<ReflectionMapShader>::operator=<ReflectionMapShader>(ReflectionMapShader* p) {
    ReflectionMapShader* raw = reinterpret_cast<ReflectionMapShader*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<Thread>&
ReferenceCountedPointer<Thread>::operator=<Thread>(Thread* p) {
    Thread* raw = reinterpret_cast<Thread*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(refVBase(p));
    DECREF(refVBase(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
Thread* ReferenceCountedPointer<Thread>::operator->() {
    return m_ptr;
}

template <>
bool ReferenceCountedPointer<Scene>::operator!=(Scene* p) const {
    return m_ptr != p;
}

template <>
Scene* ReferenceCountedPointer<Scene>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<String>::ReferenceCountedPointer(String* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
Scene& ReferenceCountedPointer<Scene>::operator*() {
    return *m_ptr;
}

template <>
template <>
ReferenceCountedPointer<Scene>&
ReferenceCountedPointer<Scene>::operator=<SceneStartup>(SceneStartup* p) {
    Scene* raw = reinterpret_cast<Scene*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(refVBase(p));
    DECREF(refVBase(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<Scene>&
ReferenceCountedPointer<Scene>::operator=<SceneMenu>(SceneMenu* p) {
    Scene* raw = reinterpret_cast<Scene*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(refVBase(p));
    DECREF(refVBase(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<Scene>&
ReferenceCountedPointer<Scene>::operator=<SceneGame>(SceneGame* p) {
    Scene* raw = reinterpret_cast<Scene*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(refVBase(p));
    DECREF(refVBase(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<m3g::Transform>&
ReferenceCountedPointer<m3g::Transform>::operator=<m3g::Transform>(m3g::Transform* p) {
    m3g::Transform* raw = reinterpret_cast<m3g::Transform*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<m3g::Image2D>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<m3g::Image2D>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
Image* ReferenceCountedPointer<Image>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<Graphics>::ReferenceCountedPointer(Graphics* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<Graphics>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
Graphics* ReferenceCountedPointer<Graphics>::operator->() {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<m3g::Image2D>&
ReferenceCountedPointer<m3g::Image2D>::operator=<m3g::Image2D>(m3g::Image2D* p) {
    m3g::Image2D* raw = reinterpret_cast<m3g::Image2D*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<m3g::Node>&
ReferenceCountedPointer<m3g::Node>::operator=<m3g::Node>(m3g::Node* p) {
    m3g::Node* raw = reinterpret_cast<m3g::Node*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
m3g::Node* ReferenceCountedPointer<m3g::Node>::operator->() {
    return m_ptr;
}

template <>
m3g::Transform* ReferenceCountedPointer<m3g::Transform>::operator->() {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<m3g::Appearance>&
ReferenceCountedPointer<m3g::Appearance>::operator=<m3g::Appearance>(m3g::Appearance* p) {
    m3g::Appearance* raw = reinterpret_cast<m3g::Appearance*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<m3g::Texture2D>&
ReferenceCountedPointer<m3g::Texture2D>::operator=<m3g::Texture2D>(m3g::Texture2D* p) {
    m3g::Texture2D* raw = reinterpret_cast<m3g::Texture2D*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
m3g::Texture2D* ReferenceCountedPointer<m3g::Texture2D>::operator->() {
    return m_ptr;
}

template <>
m3g::Appearance* ReferenceCountedPointer<m3g::Appearance>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<m3g::Node>&
ReferenceCountedPointer<m3g::Node>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<m3g::Appearance>&
ReferenceCountedPointer<m3g::Appearance>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<m3g::Texture2D>&
ReferenceCountedPointer<m3g::Texture2D>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
bool ReferenceCountedPointer<m3g::Texture2D>::operator!=(m3g::Texture2D* p) const {
    return m_ptr != p;
}

template <>
ReferenceCountedPointer<OutputStream>::ReferenceCountedPointer(OutputStream* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<OutputStream>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
bool ReferenceCountedPointer<OutputStream>::operator!=(OutputStream* p) const {
    return m_ptr != p;
}

template <>
ReferenceCountedPointer<InputStream>::ReferenceCountedPointer(InputStream* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<InputStream>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
bool ReferenceCountedPointer<InputStream>::operator!=(InputStream* p) const {
    return m_ptr != p;
}

template <>
String* ReferenceCountedPointer<String>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<ReflectionMapShader>::operator ReflectionMapShader*() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameCamera>::operator GameCamera*() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameObjectPlayer>::operator GameObjectPlayer*() const {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<GameObjectPlayer>&
ReferenceCountedPointer<GameObjectPlayer>::operator=<GameObjectPlayer>(GameObjectPlayer* p) {
    GameObjectPlayer* raw = reinterpret_cast<GameObjectPlayer*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<DataCameraAnim>::operator DataCameraAnim*() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameScreen>::operator GameScreen*() const {
    return m_ptr;
}

template <>
GameScreen* ReferenceCountedPointer<GameScreen>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameObjectGhost>::ReferenceCountedPointer(GameObjectGhost* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<GameObjectGhost>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
bool ReferenceCountedPointer<Thread>::operator==(Thread* p) const {
    return m_ptr == p;
}

template <>
template <>
ReferenceCountedPointer<GameScreen>&
ReferenceCountedPointer<GameScreen>::operator=<GameScreen>(GameScreen* p) {
    GameScreen* raw = reinterpret_cast<GameScreen*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
bool ReferenceCountedPointer<GameObjectGhost>::operator!=(GameObjectGhost* p) const {
    return m_ptr != p;
}

template <>
ReferenceCountedPointer<GameObjectGhost>::operator GameObjectGhost*() const {
    return m_ptr;
}

template <>
bool ReferenceCountedPointer<GameObjectGhost>::operator==(GameObjectGhost* p) const {
    return m_ptr == p;
}

template <>
template <>
ReferenceCountedPointer<GameObjectGhost>&
ReferenceCountedPointer<GameObjectGhost>::operator=<GameObjectGhost>(GameObjectGhost* p) {
    GameObjectGhost* raw = reinterpret_cast<GameObjectGhost*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
GameObjectGhost* ReferenceCountedPointer<GameObjectGhost>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameObjectGhost>&
ReferenceCountedPointer<GameObjectGhost>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
bool ReferenceCountedPointer<InputStream>::operator==(InputStream* p) const {
    return m_ptr == p;
}

template <>
InputStream* ReferenceCountedPointer<InputStream>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameScreen>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameScreen>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<GameScreen>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
AnimPlayer3D* ReferenceCountedPointer<AnimPlayer3D>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<String>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
template <>
ReferenceCountedPointer<String>&
ReferenceCountedPointer<String>::operator=<String>(String* p) {
    String* raw = reinterpret_cast<String*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<m3g::World>::ReferenceCountedPointer(m3g::World* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<m3g::World>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::Camera>::ReferenceCountedPointer(m3g::Camera* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<m3g::Camera>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<AnimPlayer3D>::ReferenceCountedPointer(AnimPlayer3D* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<AnimPlayer3D>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<m3g::World>&
ReferenceCountedPointer<m3g::World>::operator=<m3g::World>(m3g::World* p) {
    m3g::World* raw = reinterpret_cast<m3g::World*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<m3g::Camera>&
ReferenceCountedPointer<m3g::Camera>::operator=<m3g::Camera>(m3g::Camera* p) {
    m3g::Camera* raw = reinterpret_cast<m3g::Camera*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<AnimPlayer3D>&
ReferenceCountedPointer<AnimPlayer3D>::operator=<AnimPlayer3D>(AnimPlayer3D* p) {
    AnimPlayer3D* raw = reinterpret_cast<AnimPlayer3D*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
bool ReferenceCountedPointer<Thread>::operator!=(Thread* p) const {
    return m_ptr != p;
}

template <>
ReferenceCountedPointer<Thread>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(refVBase(m_ptr));
}

template <>
m3g::Camera* ReferenceCountedPointer<m3g::Camera>::operator->() {
    return m_ptr;
}

template <>
bool ReferenceCountedPointer<m3g::World>::operator!=(m3g::World* p) const {
    return m_ptr != p;
}

template <>
OutputStream* ReferenceCountedPointer<OutputStream>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<File>::ReferenceCountedPointer(File* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<File>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
File* ReferenceCountedPointer<File>::operator->() {
    return m_ptr;
}

template <>
bool ReferenceCountedPointer<String>::operator==(String* p) const {
    return m_ptr == p;
}

template <>
bool ReferenceCountedPointer<String>::operator!=(String* p) const {
    return m_ptr != p;
}

template <>
ReferenceCountedPointer<SharedALBuffer>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<SharedALBuffer>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<SharedALBuffer>&
ReferenceCountedPointer<SharedALBuffer>::operator=<SharedALBuffer>(SharedALBuffer* p) {
    SharedALBuffer* raw = reinterpret_cast<SharedALBuffer*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<SharedALBuffer>::ReferenceCountedPointer(SharedALBuffer* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<SharedALBuffer>&
ReferenceCountedPointer<SharedALBuffer>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
SharedALBuffer* ReferenceCountedPointer<SharedALBuffer>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<CollShape>::operator CollShape*() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameObjectCulledGraphic>::operator GameObjectCulledGraphic*() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<m3g::Group>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<m3g::Group>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::Node>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
template <>
ReferenceCountedPointer<m3g::Group>&
ReferenceCountedPointer<m3g::Group>::operator=<m3g::Group>(m3g::Group* p) {
    m3g::Group* raw = reinterpret_cast<m3g::Group*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<GameObject>::ReferenceCountedPointer(GameObjectCollectable* p) : m_ptr(reinterpret_cast<GameObject*>(p)) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<GameObject>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<GameObject>::operator GameObject*() const {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<DataCameraAnim>&
ReferenceCountedPointer<DataCameraAnim>::operator=<DataCameraAnim>(DataCameraAnim* p) {
    DataCameraAnim* raw = reinterpret_cast<DataCameraAnim*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
bool ReferenceCountedPointer<m3g::Node>::operator!=(m3g::Node* p) const {
    return m_ptr != p;
}

template <>
ReferenceCountedPointer<m3g::Group>::ReferenceCountedPointer(m3g::Group* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<m3g::Group>&
ReferenceCountedPointer<m3g::Group>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<DataCameraAnim>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<DataCameraAnim>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<DataCameraAnim>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<Player>::ReferenceCountedPointer(Player* p) : m_ptr(p) {
    INCREF(refVBase(p));
}

template <>
ReferenceCountedPointer<Player>::~ReferenceCountedPointer() {
    DECREF(refVBase(m_ptr));
}

template <>
bool ReferenceCountedPointer<Player>::operator!=(Player* p) const {
    return m_ptr != p;
}

template <>
Player* ReferenceCountedPointer<Player>::operator->() {
    return m_ptr;
}

template <>
CollShape* ReferenceCountedPointer<CollShape>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<CollShape>::ReferenceCountedPointer(CollShape* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<CollShape>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<GameObjectCulledGraphic>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameObjectCulledGraphic>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<CollShape>&
ReferenceCountedPointer<CollShape>::operator=<CollOrthoHexahedron>(CollOrthoHexahedron* p) {
    CollShape* raw = reinterpret_cast<CollShape*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
bool ReferenceCountedPointer<GameObjectCulledGraphic>::operator!=(GameObjectCulledGraphic* p) const {
    return m_ptr != p;
}

template <>
GameObjectCulledGraphic* ReferenceCountedPointer<GameObjectCulledGraphic>::operator->() {
    return m_ptr;
}

template <>
bool ReferenceCountedPointer<CollShape>::operator!=(CollShape* p) const {
    return m_ptr != p;
}

template <>
template <>
ReferenceCountedPointer<m3g::Node>&
ReferenceCountedPointer<m3g::Node>::operator=<m3g::Group>(m3g::Group* p) {
    m3g::Node* raw = reinterpret_cast<m3g::Node*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<String>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
AnimPlayer3D* ReferenceCountedPointer<AnimPlayer3D>::operator->() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<m3g::AppearanceBase>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<m3g::AppearanceBase>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::KeyframeSequence>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<m3g::KeyframeSequence>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<m3g::AppearanceBase>&
ReferenceCountedPointer<m3g::AppearanceBase>::operator=<m3g::AppearanceBase>(m3g::AppearanceBase* p) {
    m3g::AppearanceBase* raw = reinterpret_cast<m3g::AppearanceBase*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<m3g::KeyframeSequence>&
ReferenceCountedPointer<m3g::KeyframeSequence>::operator=<m3g::KeyframeSequence>(m3g::KeyframeSequence* p) {
    m3g::KeyframeSequence* raw = reinterpret_cast<m3g::KeyframeSequence*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
GameCamera* ReferenceCountedPointer<GameCamera>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<CollShape>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<particles::ParticleEffect>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<particles::ParticleEffect>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<CollShape>&
ReferenceCountedPointer<CollShape>::operator=<CollOctahedron>(CollOctahedron* p) {
    CollShape* raw = reinterpret_cast<CollShape*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<CollShape>&
ReferenceCountedPointer<CollShape>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<GameObjectCulledGraphic>&
ReferenceCountedPointer<GameObjectCulledGraphic>::operator=<GameObjectCulledGraphic>(GameObjectCulledGraphic* p) {
    GameObjectCulledGraphic* raw = reinterpret_cast<GameObjectCulledGraphic*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
bool ReferenceCountedPointer<AnimPlayer3D>::operator!=(AnimPlayer3D* p) const {
    return m_ptr != p;
}

template <>
bool ReferenceCountedPointer<particles::ParticleEffect>::operator==(particles::ParticleEffect* p) const {
    return m_ptr == p;
}

template <>
template <>
ReferenceCountedPointer<particles::ParticleEffect>&
ReferenceCountedPointer<particles::ParticleEffect>::operator=<particles::ParticleEffect>(particles::ParticleEffect* p) {
    particles::ParticleEffect* raw = reinterpret_cast<particles::ParticleEffect*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
particles::ParticleEffect* ReferenceCountedPointer<particles::ParticleEffect>::operator->() {
    return m_ptr;
}

template <>
bool ReferenceCountedPointer<particles::ParticleEffect>::operator!=(particles::ParticleEffect* p) const {
    return m_ptr != p;
}

template <>
ReferenceCountedPointer<particles::ParticleEffect>::ReferenceCountedPointer(particles::ParticleEffect* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<particles::ParticleEffect>&
ReferenceCountedPointer<particles::ParticleEffect>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
AnimationBlender* ReferenceCountedPointer<AnimationBlender>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<AnimationBlender>::ReferenceCountedPointer(AnimationBlender* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<AnimationBlender>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<AnimationBlender>&
ReferenceCountedPointer<AnimationBlender>::operator=<AnimationBlender>(AnimationBlender* p) {
    AnimationBlender* raw = reinterpret_cast<AnimationBlender*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<GhostAnimationPlayback>::ReferenceCountedPointer(GhostAnimationPlayback* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<GhostAnimationPlayback>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
GhostAnimationPlayback* ReferenceCountedPointer<GhostAnimationPlayback>::operator->() {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<Image>&
ReferenceCountedPointer<Image>::operator=<Image>(Image* p) {
    Image* raw = reinterpret_cast<Image*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<StringBuffer>&
ReferenceCountedPointer<StringBuffer>::operator=<StringBuffer>(StringBuffer* p) {
    StringBuffer* raw = reinterpret_cast<StringBuffer*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<StringBuffer>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<StringBuffer>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<String>&
ReferenceCountedPointer<String>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<m3g::Texture2D>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
bool ReferenceCountedPointer<Image>::operator!=(Image* p) const {
    return m_ptr != p;
}

template <>
template <>
ReferenceCountedPointer<CollShape>&
ReferenceCountedPointer<CollShape>::operator=<CollShape>(CollShape* p) {
    CollShape* raw = reinterpret_cast<CollShape*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<particles::ParticleMode>&
ReferenceCountedPointer<particles::ParticleMode>::operator=<particles::ParticleMode>(particles::ParticleMode* p) {
    particles::ParticleMode* raw = reinterpret_cast<particles::ParticleMode*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<particles::Particles>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<particles::Particles>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::AnimationController>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<m3g::AnimationController>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<particles::EmissionMode>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<particles::EmissionMode>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<util::JRandom>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<util::JRandom>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<particles::Particles>&
ReferenceCountedPointer<particles::Particles>::operator=<particles::Particles>(particles::Particles* p) {
    particles::Particles* raw = reinterpret_cast<particles::Particles*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<m3g::AnimationController>&
ReferenceCountedPointer<m3g::AnimationController>::operator=<m3g::AnimationController>(m3g::AnimationController* p) {
    m3g::AnimationController* raw = reinterpret_cast<m3g::AnimationController*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<particles::EmissionMode>&
ReferenceCountedPointer<particles::EmissionMode>::operator=<particles::EmissionMode>(particles::EmissionMode* p) {
    particles::EmissionMode* raw = reinterpret_cast<particles::EmissionMode*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<util::JRandom>&
ReferenceCountedPointer<util::JRandom>::operator=<util::JRandom>(util::JRandom* p) {
    util::JRandom* raw = reinterpret_cast<util::JRandom*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<particles::Particles>&
ReferenceCountedPointer<particles::Particles>::operator=<particles::PointParticles>(particles::PointParticles* p) {
    particles::Particles* raw = reinterpret_cast<particles::Particles*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<particles::Particles>&
ReferenceCountedPointer<particles::Particles>::operator=<particles::QuadParticles>(particles::QuadParticles* p) {
    particles::Particles* raw = reinterpret_cast<particles::Particles*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<particles::Particles>&
ReferenceCountedPointer<particles::Particles>::operator=<particles::CylinderParticles>(particles::CylinderParticles* p) {
    particles::Particles* raw = reinterpret_cast<particles::Particles*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
particles::Particles* ReferenceCountedPointer<particles::Particles>::operator->() {
    return m_ptr;
}

template <>
m3g::AnimationController* ReferenceCountedPointer<m3g::AnimationController>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<particles::ParticleMode>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<particles::ParticleMode>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<Image>&
ReferenceCountedPointer<Image>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
bool ReferenceCountedPointer<Image>::operator==(Image* p) const {
    return m_ptr == p;
}

template <>
ReferenceCountedPointer<WebImage>::ReferenceCountedPointer(WebImage* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<WebImage>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
bool ReferenceCountedPointer<WebImage>::operator!=(WebImage* p) const {
    return m_ptr != p;
}

template <>
WebImage* ReferenceCountedPointer<WebImage>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<WebImage>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<WebImage>&
ReferenceCountedPointer<WebImage>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<ChunkRunnerVision>::ReferenceCountedPointer(ChunkRunnerVision* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<ChunkRunnerVision>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<ChunkRunnerVision>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<ChunkRunnerVision>::operator ChunkRunnerVision*() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<ChunkSection>::operator ChunkSection*() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<ChunkColumn>::operator ChunkColumn*() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<ChunkLayer>::operator ChunkLayer*() const {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<ChunkRunnerVision>&
ReferenceCountedPointer<ChunkRunnerVision>::operator=<ChunkRunnerVision>(ChunkRunnerVision* p) {
    ChunkRunnerVision* raw = reinterpret_cast<ChunkRunnerVision*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ChunkRunnerVision* ReferenceCountedPointer<ChunkRunnerVision>::operator->() {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<ChunkSection>&
ReferenceCountedPointer<ChunkSection>::operator=<ChunkSection>(ChunkSection* p) {
    ChunkSection* raw = reinterpret_cast<ChunkSection*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ChunkSection* ReferenceCountedPointer<ChunkSection>::operator->() {
    return m_ptr;
}

template <>
m3g::Group* ReferenceCountedPointer<m3g::Group>::operator->() {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<ChunkColumn>&
ReferenceCountedPointer<ChunkColumn>::operator=<ChunkColumn>(ChunkColumn* p) {
    ChunkColumn* raw = reinterpret_cast<ChunkColumn*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ChunkColumn* ReferenceCountedPointer<ChunkColumn>::operator->() {
    return m_ptr;
}

template <>
template <>
ReferenceCountedPointer<ChunkLayer>&
ReferenceCountedPointer<ChunkLayer>::operator=<ChunkLayer>(ChunkLayer* p) {
    ChunkLayer* raw = reinterpret_cast<ChunkLayer*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ChunkLayer* ReferenceCountedPointer<ChunkLayer>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<ChunkRunnerVision>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<ChunkSection>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<ChunkSection>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<ChunkSection>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<ChunkColumn>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<ChunkColumn>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<ChunkColumn>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<ChunkLayer>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<ChunkLayer>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<ChunkLayer>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<DataCameraAnimStep>&
ReferenceCountedPointer<DataCameraAnimStep>::operator=<DataCameraAnimStep>(DataCameraAnimStep* p) {
    DataCameraAnimStep* raw = reinterpret_cast<DataCameraAnimStep*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
DataCameraAnimStep* ReferenceCountedPointer<DataCameraAnimStep>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<DataCameraAnimStep>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<DataCameraAnimStep>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<DataCameraAnimStep>::ReferenceCountedPointer(const ReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
particles::EmissionMode* ReferenceCountedPointer<particles::EmissionMode>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameScreen>::ReferenceCountedPointer(GameScreen* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<Displayable>::ReferenceCountedPointer(Displayable* p) : m_ptr(p) {
    INCREF(refVBase(p));
}

template <>
ReferenceCountedPointer<Displayable>::~ReferenceCountedPointer() {
    DECREF(refVBase(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<Displayable>&
ReferenceCountedPointer<Displayable>::operator=<Displayable>(Displayable* p) {
    Displayable* raw = reinterpret_cast<Displayable*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(refVBase(p));
    DECREF(refVBase(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
ReferenceCountedPointer<Displayable>&
ReferenceCountedPointer<Displayable>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(refVBase(other.m_ptr));
    DECREF(refVBase(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<Image>&
ReferenceCountedPointer<Image>::operator=<ImageIPhone>(ImageIPhone* p) {
    Image* raw = reinterpret_cast<Image*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
bool ReferenceCountedPointer<Displayable>::operator!=(Displayable* p) const {
    return m_ptr != p;
}

template <>
String* ReferenceCountedPointer<String>::operator->() const {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameCamera>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameCamera>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<m3g::Camera>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameObjectPlayer>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameObjectPlayer>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<GameUI::SoundSettings>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameUI::SoundSettings>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<GameUI::EndOfLevelResults>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameUI::EndOfLevelResults>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<GameUI::ScrollingText>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameUI::ScrollingText>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
ReferenceCountedPointer<GameUI::Confirmation>::ReferenceCountedPointer() : m_ptr(0) {}

template <>
ReferenceCountedPointer<GameUI::Confirmation>::~ReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <>
template <>
ReferenceCountedPointer<GameCamera>&
ReferenceCountedPointer<GameCamera>::operator=<GameCamera>(GameCamera* p) {
    GameCamera* raw = reinterpret_cast<GameCamera*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
m3g::World* ReferenceCountedPointer<m3g::World>::operator->() {
    return m_ptr;
}

template <>
GameObjectPlayer* ReferenceCountedPointer<GameObjectPlayer>::operator->() {
    return m_ptr;
}

template <>
GameUI::ScrollingText* ReferenceCountedPointer<GameUI::ScrollingText>::operator->() {
    return m_ptr;
}

template <>
GameUI::SoundSettings* ReferenceCountedPointer<GameUI::SoundSettings>::operator->() {
    return m_ptr;
}

template <>
GameUI::EndOfLevelResults* ReferenceCountedPointer<GameUI::EndOfLevelResults>::operator->() {
    return m_ptr;
}

template <>
GameUI::Confirmation* ReferenceCountedPointer<GameUI::Confirmation>::operator->() {
    return m_ptr;
}

template <>
ReferenceCountedPointer<GameUI::SoundSettings>::ReferenceCountedPointer(GameUI::SoundSettings* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<GameUI::SoundSettings>&
ReferenceCountedPointer<GameUI::SoundSettings>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<GameUI::ScrollingText>::ReferenceCountedPointer(GameUI::ScrollingText* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<GameUI::ScrollingText>&
ReferenceCountedPointer<GameUI::ScrollingText>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
ReferenceCountedPointer<GameUI::Confirmation>::ReferenceCountedPointer(GameUI::Confirmation* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <>
ReferenceCountedPointer<GameUI::Confirmation>&
ReferenceCountedPointer<GameUI::Confirmation>::operator=(const ReferenceCountedPointer& other) {
    if (m_ptr == other.m_ptr)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(other.m_ptr));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = other.m_ptr;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<GameUI::SoundSettings>&
ReferenceCountedPointer<GameUI::SoundSettings>::operator=<GameUI::SoundSettings>(GameUI::SoundSettings* p) {
    GameUI::SoundSettings* raw = reinterpret_cast<GameUI::SoundSettings*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<GameUI::ScrollingText>&
ReferenceCountedPointer<GameUI::ScrollingText>::operator=<GameUI::ScrollingText>(GameUI::ScrollingText* p) {
    GameUI::ScrollingText* raw = reinterpret_cast<GameUI::ScrollingText*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<GameUI::Confirmation>&
ReferenceCountedPointer<GameUI::Confirmation>::operator=<GameUI::Confirmation>(GameUI::Confirmation* p) {
    GameUI::Confirmation* raw = reinterpret_cast<GameUI::Confirmation*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

template <>
template <>
ReferenceCountedPointer<GameUI::EndOfLevelResults>&
ReferenceCountedPointer<GameUI::EndOfLevelResults>::operator=<GameUI::EndOfLevelResults>(GameUI::EndOfLevelResults* p) {
    GameUI::EndOfLevelResults* raw = reinterpret_cast<GameUI::EndOfLevelResults*>(p);
    if (m_ptr == raw)
        return *this;
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    m_ptr = raw;
    return *this;
}

}
