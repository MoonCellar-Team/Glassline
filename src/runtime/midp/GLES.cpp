#include "runtime/midp/GLES.h"

#include <stddef.h>

#include "runtime/midp/GLES11.h"
#include "runtime/midp/GLES20.h"

void* operator new(size_t);
#include <new>

namespace midp {

CacheGLES11* GLES::InstanceCachedGLES11() {
    return new CacheGLES11();
}

GLES::GLES() {}

GLES::~GLES() {}

CacheGLES20* GLES::InstanceCachedGLES20() {
    return new CacheGLES20();
}

}
