#include "game/IFFReader.h"

#include <string.h>

#include "runtime/midp/ByteArrayInputStream.h"
#include "runtime/midp/array.h"

bool IFFReader::isReadComplete() {
    return m_readComplete;
}

IFFReader::IFFReader(midp::InputStream* in) : m_in(in), m_readComplete(false), m_chunkSize(0) {
    m_chunkId[4] = 0;
    readChunkHeader();
}

bool IFFReader::isIdOfCurrectChunk(const char* id) {
    return !m_readComplete && strncmp(m_chunkId, id, 4) == 0;
}

void IFFReader::readChunkHeader() {
    if (m_in.available() > 7) {
        m_in.read((signed char*)m_chunkId, 4);
        m_chunkSize = m_in.readInt();
        if (m_in.available() >= m_chunkSize) {
            return;
        }
    }
    m_readComplete = true;
}

void IFFReader::skipChunk() {
    if (m_readComplete) {
        return;
    }
    m_in.skip(m_chunkSize + (m_chunkSize & 1));
    readChunkHeader();
}

midp::InputStream* IFFReader::readChunk() {
    if (m_readComplete) {
        return NULL;
    }
    midp::array<signed char> data(m_chunkSize);
    m_in.readFully(data);
    if ((m_chunkSize & 1) && m_in.available() != 0) {
        m_in.skip(1);
    }
    readChunkHeader();
    return new midp::ByteArrayInputStream(data);
}

IFFReader::~IFFReader() {
    m_in.~DataInputStream();
}
