#include "m3g/Image2D.h"
#include "m3g/Image2DClass.h"
#include "m3g/Image2DPlatformData.h"
#include "m3g/GLHelpers.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/DisplayIPhone.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/GLES.h"
extern "C" void* valloc(__SIZE_TYPE__);
extern "C" void free(void*);
namespace m3g {
void Image2DFlushGLES(midp::GLES11*,Image2D*); void Image2DFlushGLES20(midp::GLES20*,Image2D*);
namespace {
int armShift(int value,unsigned shift) { shift&=255; return shift>=32 ? (value<0 ? -1 : 0) : value>>shift; }
int armLeft(unsigned value,unsigned shift) { shift&=255; return shift>=32 ? 0 : static_cast<int>(value<<shift); }
unsigned armRight(unsigned value,unsigned shift) { shift&=255; return shift>=32 ? 0 : value>>shift; }
}
}

namespace m3g {

Image2D::Image2D() : m_platform(new Image2DPlatformData),m_format(0),m_width(0),m_height(0),m_mutable(true),m_bitsPerPixel(0),m_levels(0),m_data(nullptr),m_dataLevels(0) {}

Image2D::Image2D(int format,midp::Image* image) : m_platform(new Image2DPlatformData),m_format(0),m_width(0),m_height(0),m_mutable(true),m_bitsPerPixel(0),m_levels(0),m_data(nullptr),m_dataLevels(0) {
    midp::ReferenceCountedHardReference reference(image); set(format,image);
}

Image2D::~Image2D() { discard(); delete m_platform; m_platform=nullptr; }

midp::Class* Image2D::getClass() const { return new Image2DClass; }

int Image2D::getM3GUniqueClassID() const { return 10; }

void* Image2D::getMipMapData(int level) const { int index=level<m_dataLevels ? level : m_dataLevels-1; return m_data[index]; }

int Image2D::getMipMapWidth(int level) const { int value=armShift(m_width,level); return value ? value : 1; }

int Image2D::getMipMapHeight(int level) const { int value=armShift(m_height,level); return value ? value : 1; }

int Image2D::getMipMapDataSize(int level) const {
    int index=level<m_dataLevels ? level : m_dataLevels-1;
    int width=getMipMapWidth(index), height=getMipMapHeight(index);
    int bits=height*width*m_bitsPerPixel+7, bytes=bits/8;
    if(static_cast<unsigned>(m_format-0x7a)<4) { if(bits<256) bytes=32; }
    else if(m_format==0x79) { if(bits<128) bytes=16; }
    else if(m_format==0x78) { if(bits<64) bytes=8; }
    return bytes;
}

bool Image2D::isMutable() const { return m_mutable; }

void Image2D::discard() {
    for(int i=0;i<m_dataLevels;++i) { unsigned char* data=m_data[i]; m_data[i]=nullptr; delete[] data; }
    if(m_data) { delete[] m_data; m_data=nullptr; } m_dataLevels=0;
}

void Image2D::duplicateTo(Object3D* target) {
    Object3D::duplicateTo(target); Image2D* result=static_cast<Image2D*>(target);
    result->m_format=m_format; result->m_width=m_width; result->m_height=m_height; result->m_mutable=m_mutable; result->m_bitsPerPixel=m_bitsPerPixel;
}

void Image2D::set(int format,int width,int height) {
    m_bitsPerPixel=0; m_format=format; m_width=width; m_height=height;
    switch(format&255) {
    case 0x60: case 0x61: case 0x79: m_bitsPerPixel=8; break;
    case 0x62: case 0x65: case 0x66: case 0x67: m_bitsPerPixel=16; break;
    case 99: m_bitsPerPixel=24; break; case 100: m_bitsPerPixel=32; break;
    case 0x78: case 0x7c: case 0x7d: m_bitsPerPixel=4; break;
    case 0x7a: case 0x7b: m_bitsPerPixel=2; break;
    default: m_mutable=true; return;
    }
    int levels=1;
    if(!(format&0x8000)) { levels=0; while(m_width>=armLeft(1,levels) || m_height>=armLeft(1,levels)) ++levels; }
    m_levels=levels; m_dataLevels=levels; m_data=new unsigned char*[levels];
    for(int i=0;i<m_dataLevels;++i) m_data[i]=new unsigned char[getMipMapDataSize(i)];
    m_mutable=true;
}

void Image2D::commit(const midp::array<signed char>& palette,const signed char* indices) {
    int channels=m_format==100 ? 4 : (m_format==99 ? 3 : 0);
    if(channels) for(int level=0;level<getNumMipMapLevels();++level) {
        unsigned char* out=static_cast<unsigned char*>(getMipMapData(level));
        int width=getMipMapWidth(level), height=getMipMapHeight(level), block=armLeft(1,level);
        for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
            unsigned sums[4]={0,0,0,0};
            for(int by=0;by<block;++by) for(int bx=0;bx<block;++bx) {
                unsigned index=static_cast<unsigned char>(indices[m_width*(armLeft(y,level)+by)+armLeft(x,level)+bx]);
                for(int c=0;c<channels;++c) sums[c]+=static_cast<unsigned char>(palette[index*channels+c]);
            }
            for(int c=0;c<channels;++c) *out++=static_cast<unsigned char>(armRight(sums[c],level*2));
        }
    }
    m_mutable=false;
}

void Image2D::commit(const unsigned char* pixels) {
    unsigned format=static_cast<unsigned char>(m_format);
#ifdef MEDECOMP_PORT
    if(format==99 || format==100) {
        int channels=format==100 ? 4 : 3;
        __builtin_memcpy(getMipMapData(0),pixels,static_cast<size_t>(m_width)*m_height*channels);
        for(int level=1;level<getNumMipMapLevels();++level) {
            const unsigned char* prev=static_cast<const unsigned char*>(getMipMapData(level-1));
            unsigned char* out=static_cast<unsigned char*>(getMipMapData(level));
            int pw=getMipMapWidth(level-1), ph=getMipMapHeight(level-1);
            int w=getMipMapWidth(level), h=getMipMapHeight(level);
            int sx=pw>w ? 2 : 1, sy=ph>h ? 2 : 1;
            for(int y=0;y<h;++y) for(int x=0;x<w;++x) for(int c=0;c<channels;++c) {
                unsigned sum=0;
                for(int by=0;by<sy;++by) for(int bx=0;bx<sx;++bx) sum+=prev[((y*sy+by)*pw+x*sx+bx)*channels+c];
                *out++=static_cast<unsigned char>(sum/(sx*sy));
            }
        }
        m_mutable=false;
        return;
    }
#endif
    if(format==99 || format==100 || format==101) {
        int inputChannels=format==100 ? 4 : 3;
        int horizontal=0, vertical=0;
        for(int level=0;level<getNumMipMapLevels();++level) {
            unsigned char* output=static_cast<unsigned char*>(getMipMapData(level));
            int width=getMipMapWidth(level), height=getMipMapHeight(level);
            if(level>0) {
                int oldHeight=getMipMapHeight(level-1), oldWidth=getMipMapWidth(level-1);
                if(oldHeight!=height) vertical=level; if(oldWidth!=width) horizontal=level;
            }
            int blockHeight=armLeft(1,vertical), blockWidth=armLeft(1,horizontal);
            int shift=horizontal+vertical;
            for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
                unsigned sums[4]={0,0,0,0};
                for(int by=0;by<blockHeight;++by) for(int bx=0;bx<blockWidth;++bx) {
                    const unsigned char* source=pixels+((armLeft(y,vertical)+by)*m_width+armLeft(x,horizontal)+bx)*inputChannels;
                    for(int c=0;c<inputChannels;++c) sums[c]+=source[c];
                }
                if(format==101) {
                    unsigned r=armRight(sums[0],shift), g=armRight(sums[1],shift), b=armRight(sums[2],shift);
                    unsigned short packed=static_cast<unsigned short>(((g<<3)&0x7e0)|((r<<8)&0xf800)|((b<<24)>>27));
                    *reinterpret_cast<unsigned short*>(output)=packed; output+=2;
                } else for(int c=0;c<inputChannels;++c) *output++=static_cast<unsigned char>(armRight(sums[c],shift));
            }
        }
    } else if(format==0x78 || format==0x79) {
        int offset=12;
        for(int i=0;i<getNumMipMapLevels();++i) { int size=getMipMapDataSize(i); if(size>0) __builtin_memcpy(getMipMapData(i),pixels+offset,size); offset+=size+12; }
    } else if(format==0x7c || format==0x7d) {
        int offset=0;
        for(int i=0;i<getNumMipMapLevels();++i) { void* out=getMipMapData(i); int size=getMipMapDataSize(i), width=getMipMapWidth(i); __builtin_memcpy(out,pixels+offset,size); if(width>=9) offset+=size; }
    }
    m_mutable=false;
}

void Image2D::set(int format,const midp::Image* image) {
    int width=image->getWidth(), height=image->getHeight(), count=width*height; set(format,width,height);
    midp::array<int> argb(count); const_cast<midp::Image*>(image)->getRGB(argb, 0, width, 0, 0, width, height);
    if(format!=99 && format!=100 && format!=101) return;
    int channels=format==100 ? 4 : 3;
    unsigned char* data=static_cast<unsigned char*>(valloc(count*channels));
    for(int i=0;i<count;++i) { unsigned color=argb[i]; data[i*channels]=color>>16; data[i*channels+1]=color>>8; data[i*channels+2]=color; if(channels==4) data[i*channels+3]=color>>24; }
    commit(data); free(data);
}

void Image2D::uploadImage() {
    if(m_platform->texture && !m_platform->dirty) return;
    void* gl=midp::DisplayIPhone::getStaticDisplay()->getGL(); int version=static_cast<midp::GLES*>(gl)->getGLESVersion();
    gl=midp::DisplayIPhone::getStaticDisplay()->getGL();
    if(version==2) {
        Image2DBindGLES20(static_cast<midp::GLES20*>(gl),this,0xde1,0x2601,0x2601,0x2901,0x2901);
        gl=midp::DisplayIPhone::getStaticDisplay()->getGL(); Image2DFlushGLES20(static_cast<midp::GLES20*>(gl),this);
    } else {
        Image2DBindGLES(static_cast<midp::GLES11*>(gl),this,0xde1,0x2601,0x2601,0x2901,0x2901);
        gl=midp::DisplayIPhone::getStaticDisplay()->getGL(); Image2DFlushGLES(static_cast<midp::GLES11*>(gl),this);
    }
}

int Image2D::getNumMipMapLevels() const { return m_levels; }

int Image2D::getFormat() const { return m_format; }

int Image2D::getWidth() const { return m_width; }

int Image2D::getHeight() const { return m_height; }

}
