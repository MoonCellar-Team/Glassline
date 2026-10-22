#include "runtime/midp/ReferenceCountedPointer.h"

namespace particles {
class EmissionMode;
class ParticleEffect;
class ParticleMode;
class Particles;
}

template midp::ReferenceCountedPointer<particles::ParticleMode>::operator particles::ParticleMode*() const;

template midp::ReferenceCountedPointer<particles::EmissionMode>::operator particles::EmissionMode*() const;

template midp::ReferenceCountedPointer<particles::Particles>::operator particles::Particles*() const;

template midp::ReferenceCountedPointer<particles::ParticleEffect>::operator particles::ParticleEffect*() const;
