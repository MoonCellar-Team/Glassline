#pragma once
#include "runtime/midp/ReferenceCounted.h"
namespace platform {
inline midp::ReferenceCounted* referenceBase(void* object) {
    if(!object) return nullptr;
    int* table=*reinterpret_cast<int**>(object);
    return reinterpret_cast<midp::ReferenceCounted*>(static_cast<char*>(object)+table[-3]);
}
template<class R,class... A> inline R call(void* object,unsigned offset,A... args) {
    using F=R (*)(void*,A...);
    return reinterpret_cast<F>((*static_cast<void***>(object))[offset/4])(object,args...);
}
}
