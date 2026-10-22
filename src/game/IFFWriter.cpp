#include "game/IFFWriter.h"

#include <stddef.h>

#include "runtime/midp/ByteArrayOutputStream.h"

void IFFWriter::storeCurrentChunk() {
    int size = m_chunkData->size();
    if (size == 0) {
        return;
    }
    m_out.write(m_chunkId, 4);
    m_out.writeInt(size);
    m_chunkData->writeTo(reinterpret_cast<midp::OutputStream*>(&m_out));
    if (size & 1) {
        m_out.write((unsigned char)0);
    }
    m_chunkData->reset();
}

midp::DataOutputStream* IFFWriter::writeChunk(const char* id) {
    storeCurrentChunk();
    int src = 0;
    for (int i = 0; i < 4; i++) {
        if (id[src] == 0) {
            m_chunkId[i] = ' ';
        } else {
            m_chunkId[i] = id[src++];
        }
    }
    m_chunkId[4] = 0;
    return m_chunkOut;
}

IFFWriter::~IFFWriter() {
    storeCurrentChunk();
    midp::DECREF(m_chunkData);
    m_chunkData = NULL;
    midp::DECREF(m_chunkOut);
    m_chunkOut = NULL;
    m_out.~DataOutputStream();
}

IFFWriter::IFFWriter(midp::OutputStream* out) : m_out(out), m_chunkData(NULL), m_chunkOut(NULL) {
    m_chunkData = new midp::ByteArrayOutputStream();
    midp::INCREF(m_chunkData);
    m_chunkOut = new midp::DataOutputStream(m_chunkData);
    midp::INCREF(m_chunkOut);
}
