#include "m3g/CustomShader.h"
namespace midp {
class DisplayIPhone { public: static DisplayIPhone* getStaticDisplay(); void* getGL(); };
}

namespace m3g {

CustomShader::CustomShader() : m_gl(nullptr) { m_gl=midp::DisplayIPhone::getStaticDisplay()->getGL(); }

CustomShader::~CustomShader() {}

void CustomShader::setScaledWorld(const Transform&) {}

void CustomShader::setView(const Transform&) {}

}

void* operator new[](__SIZE_TYPE__);
void operator delete[](void*) noexcept;

void* __raw_alloc(unsigned size) { return ::operator new[](size); }

void __raw_dealloc(void* memory) { if(memory) ::operator delete[](memory); }

void transpose(float* matrix) {
    const int pairs[6][2]={{1,4},{2,8},{3,12},{6,9},{7,13},{11,14}};
    for(int i=0;i<6;++i) { float value=matrix[pairs[i][0]]; matrix[pairs[i][0]]=matrix[pairs[i][1]]; matrix[pairs[i][1]]=value; }
}
