#pragma once
#include "runtime/midp/InputStream.h"

namespace m3g {

class ChunkInputStream : public midp::InputStream {
public:
    ChunkInputStream(midp::InputStream* input, int length);
    virtual ~ChunkInputStream();
    virtual int available() const;
    virtual int read();
    virtual int read(signed char* bytes, int length);
    virtual int skip(long length);
    midp::InputStream* m_input;
    int m_length;
    int m_position;
};

}
