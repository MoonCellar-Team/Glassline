#include "m3g/ChunkInputStream.h"

namespace m3g {

ChunkInputStream::ChunkInputStream(midp::InputStream* input, int length) : m_input(input), m_length(length), m_position(0) {}

ChunkInputStream::~ChunkInputStream() {}

int ChunkInputStream::available() const { return m_length-m_position; }

int ChunkInputStream::read() { if(m_position>=m_length) return -1; ++m_position; return m_input->read(); }

int ChunkInputStream::skip(long length) {
    int remaining=m_length-m_position;
    int count=length < remaining ? length : remaining;
    m_input->skip(count); m_position+=count; return count;
}

int ChunkInputStream::read(signed char* bytes, int length) {
    if(m_position>=m_length) return -1;
    int remaining=m_length-m_position;
    int count=m_input->read(bytes, length < remaining ? length : remaining);
    if(count>0) m_position+=count;
    return count;
}

}
