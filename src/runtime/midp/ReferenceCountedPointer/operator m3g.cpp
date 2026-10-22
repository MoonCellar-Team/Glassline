#include "runtime/midp/ReferenceCountedPointer.h"

namespace m3g {
class AppearanceBase;
class Camera;
class Group;
class Image2D;
class KeyframeSequence;
class Node;
class Texture2D;
class Transform;
class World;
}

template midp::ReferenceCountedPointer<m3g::Node>::operator m3g::Node*() const;

template midp::ReferenceCountedPointer<m3g::Transform>::operator m3g::Transform*() const;

template midp::ReferenceCountedPointer<m3g::Image2D>::operator m3g::Image2D*() const;

template midp::ReferenceCountedPointer<m3g::Texture2D>::operator m3g::Texture2D*() const;

template midp::ReferenceCountedPointer<m3g::World>::operator m3g::World*() const;

template midp::ReferenceCountedPointer<m3g::Group>::operator m3g::Group*() const;

template midp::ReferenceCountedPointer<m3g::AppearanceBase>::operator m3g::AppearanceBase*() const;

template midp::ReferenceCountedPointer<m3g::KeyframeSequence>::operator m3g::KeyframeSequence*() const;

template midp::ReferenceCountedPointer<m3g::Camera>::operator m3g::Camera*() const;
