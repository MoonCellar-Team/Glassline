#pragma once

#include "runtime/midp/DataOutputStream.h"

namespace midp {
class ByteArrayOutputStream;
class OutputStream;
}

class IFFWriter {
public:
    explicit IFFWriter(midp::OutputStream* out);
    ~IFFWriter();

    midp::DataOutputStream* writeChunk(const char* id);
    void storeCurrentChunk();

    char m_chunkId[5];
    union { midp::DataOutputStream m_out; char m_outBytes[0xc]; };
    midp::ByteArrayOutputStream* m_chunkData;
    midp::DataOutputStream* m_chunkOut;
};
