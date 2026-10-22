#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Displayable;
class Graphics;
class Image;
class InputStream;
class OutputStream;
class SharedALBuffer;
class String;
class StringBuffer;
class WebImage;
}

template midp::ReferenceCountedPointer<midp::String>::operator midp::String*() const;

template midp::ReferenceCountedPointer<midp::StringBuffer>::operator midp::StringBuffer*() const;

template midp::ReferenceCountedPointer<midp::Graphics>::operator midp::Graphics*() const;

template midp::ReferenceCountedPointer<midp::Image>::operator midp::Image*() const;

template midp::ReferenceCountedPointer<midp::OutputStream>::operator midp::OutputStream*() const;

template midp::ReferenceCountedPointer<midp::InputStream>::operator midp::InputStream*() const;

template midp::ReferenceCountedPointer<midp::SharedALBuffer>::operator midp::SharedALBuffer*() const;

template midp::ReferenceCountedPointer<midp::WebImage>::operator midp::WebImage*() const;

template midp::ReferenceCountedPointer<midp::Displayable>::operator midp::Displayable*() const;
