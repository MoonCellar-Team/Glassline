#include "m3g/GZipStreamBasic.h"
#include <stddef.h>
extern "C" int inflate(void*,int);
extern "C" int inflateInit2_(void*,int,const char*,int);
extern "C" int inflateEnd(void*);

namespace m3g {

static_assert(sizeof(void*) != 4 || offsetof(GZipStreamBasic, m_blockSize) - offsetof(GZipStreamBasic, m_input) == 0x3c, "gzip от m_input");
static_assert(sizeof(void*) != 4 || offsetof(GZipStreamBasic, m_blockSize) - offsetof(GZipStreamBasic, m_nextIn) == 0x38, "gzip z_stream");
static_assert(offsetof(GZipStreamBasic, m_blockSize) - offsetof(GZipStreamBasic, m_zState) == 32, "m_zState");

GZipStreamBasic::GZipStreamBasic() : m_blockSize(0x4000),m_compressed(nullptr),m_decompressed(nullptr),m_position(0),m_length(0),m_eof(false) {
    __builtin_memset(&m_input,0,offsetof(GZipStreamBasic, m_blockSize) - offsetof(GZipStreamBasic, m_input));
    __builtin_memset(&m_nextIn,0,offsetof(GZipStreamBasic, m_blockSize) - offsetof(GZipStreamBasic, m_nextIn));
}

GZipStreamBasic::~GZipStreamBasic() { gzclose(); }

int GZipStreamBasic::available() const {
    while(m_position==m_length || m_length==0) {
        if(m_eof) return 0;
        decompressBlock();
    }
    return m_length-m_position;
}

int GZipStreamBasic::read() {
    while(m_position==m_length || m_length==0) { if(m_eof) return -1; decompressBlock(); }
    return m_decompressed[m_position++];
}

int GZipStreamBasic::read(signed char* bytes,int count) {
    int total=0;
    while(count!=0) {
        while(m_position==m_length || m_length==0) { if(m_eof) return total; decompressBlock(); }
        int take=m_length-m_position; if(count<take) take=count;
        count-=take; __builtin_memcpy(bytes+total,m_decompressed+m_position,take); total+=take; m_position+=take;
    }
    return total;
}

int GZipStreamBasic::skip(long count) {
    int total=0;
    while(count!=0) {
        while(m_position==m_length || m_length==0) { if(m_eof) return total; decompressBlock(); }
        int take=m_length-m_position; if(count<take) take=count;
        total+=take; count-=take; m_position+=take;
    }
    return total;
}

void GZipStreamBasic::decompressBlock() const {
    if(m_eof) return;
    if(m_availIn==0) {
        unsigned available=static_cast<unsigned>(m_input->available()); unsigned count=m_blockSize;
        if(available<=count) {
            count=available;
            if(available==static_cast<unsigned>(m_blockSize)) m_eof=true;
            else if(available==0) { m_eof=true; return; }
        }
        m_input->read(reinterpret_cast<signed char*>(m_compressed),count);
        m_nextIn=m_compressed; m_availIn=count;
    }
    m_totalOut=0; m_nextOut=m_decompressed; m_availOut=m_blockSize;
    int result=inflate(&m_nextIn,0); if(result==1) m_eof=true;
    m_position=0; m_length=m_totalOut;
}

bool GZipStreamBasic::gzopen(midp::DataInputStream* input) {
    if(m_input) gzclose();
    if(input->available()<10) return false;
    int first=input->readUnsignedByte(); int second=input->readUnsignedByte();
    if(static_cast<unsigned char>(first)!=0x1f || static_cast<unsigned char>(second)!=0x8b) return false;
    int method=input->readUnsignedByte(); int flags=input->readUnsignedByte();
    if(static_cast<unsigned char>(method)!=8 || (method&1)!=0) return false;
    input->skip(6);
    if(flags&4) { int low=input->readUnsignedByte(); int high=input->readUnsignedByte(); input->skip(low+high*256); }
    if(flags&8) while(input->readUnsignedByte()!=0) {}
    if(flags&16) while(input->readUnsignedByte()!=0) {}
    if(flags&2) input->skip(2);
    if(input->available()<=0) return false;
    *reinterpret_cast<unsigned*>(m_zState+8)=0; *reinterpret_cast<unsigned*>(m_zState+12)=0; *reinterpret_cast<unsigned*>(m_zState+16)=0;
    m_nextIn=nullptr; m_nextOut=nullptr; m_availIn=0;
    if(inflateInit2_(&m_nextIn,-15,"1.2.5",0x38)!=0) return false;
    m_input=input; m_decompressed=new unsigned char[m_blockSize]; m_compressed=new unsigned char[m_blockSize];
    decompressBlock(); return true;
}

void GZipStreamBasic::gzclose() {
    delete[] m_decompressed; m_decompressed=nullptr;
    delete[] m_compressed; m_compressed=nullptr;
    if(m_input) inflateEnd(&m_nextIn);
    m_position=0; m_eof=false; m_length=0;
    __builtin_memset(&m_input,0,offsetof(GZipStreamBasic, m_blockSize) - offsetof(GZipStreamBasic, m_input));
}

}
