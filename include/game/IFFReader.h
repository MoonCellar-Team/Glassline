#pragma once

#include "runtime/midp/DataInputStream.h"

namespace midp {
class InputStream;
}

class IFFReader {
public:
    explicit IFFReader(midp::InputStream* in);
    ~IFFReader();

    bool isReadComplete();
    bool isIdOfCurrectChunk(const char* id);
    midp::InputStream* readChunk();
    void skipChunk();
    void readChunkHeader();

    union { midp::DataInputStream m_in; char m_inBytes[0x10]; };
    bool m_readComplete;
    char m_chunkId[5];
    int m_chunkSize;
};
