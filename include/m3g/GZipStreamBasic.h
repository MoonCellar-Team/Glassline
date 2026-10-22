#pragma once
#include "runtime/midp/InputStream.h"
#include "runtime/midp/DataInputStream.h"

namespace m3g {

class GZipStreamBasic : public midp::InputStream {
public:
    GZipStreamBasic();
    virtual ~GZipStreamBasic();
    virtual int available() const;
    virtual int read();
    virtual int read(signed char* bytes,int count);
    virtual int skip(long count);
    bool gzopen(midp::DataInputStream* input);
    virtual void gzclose();
    void decompressBlock() const;
    mutable midp::DataInputStream* m_input;
    mutable unsigned char* m_nextIn;
    mutable unsigned m_availIn;
    mutable unsigned m_totalIn;
    mutable unsigned char* m_nextOut;
    mutable unsigned m_availOut;
    mutable unsigned m_totalOut;
    mutable unsigned char m_zState[32];
    int m_blockSize;
    unsigned char* m_compressed;
    unsigned char* m_decompressed;
    mutable int m_position;
    mutable int m_length;
    mutable bool m_eof;
};

}
