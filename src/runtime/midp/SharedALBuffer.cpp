#include "runtime/midp/SharedALBuffer.h"

#include "platform/AudioLock.h"
#include "runtime/midp/String.h"
#include "sdk/string.h"

extern "C" {

enum {
    kAudioFilePropertyDataFormat = 0x64666d74,
    kAudioFilePropertyAudioDataByteCount = 0x62636e74
};

enum {
    AL_FORMAT_MONO8 = 0x1100,
    AL_FORMAT_MONO16 = 0x1101,
    AL_FORMAT_STEREO8 = 0x1102,
    AL_FORMAT_STEREO16 = 0x1103
};

int AudioFileClose(OpaqueAudioFileID* file);
int AudioFileGetProperty(OpaqueAudioFileID* file, unsigned int property, unsigned int* size, void* data);
int AudioFileReadBytes(OpaqueAudioFileID* file, unsigned char useCache, long long start, unsigned int* count,
                       void* buffer);
void alGenBuffers(int count, unsigned int* buffers);
void alDeleteBuffers(int count, unsigned int* buffers);
void (*alBufferDataStatic)(unsigned int buffer, int format, const void* data, int size, int freq);

}

void* operator new(size_t);
void* operator new[](size_t);

namespace midp {

const void* SharedALBuffer::getAudioData() const {
    return m_audioData;
}

void SharedALBuffer::close() {
    if (m_file == 0)
        return;
    AudioFileClose(m_file);
    m_file = 0;
}

void SharedALBuffer::invalidate() {
    if (!isRealized())
        return;
    alDeleteBuffers(1, &m_buffer);
    m_buffer = 0;
}

void SharedALBuffer::loadAudioData() {
    if (isAudioDataLoaded())
        return;
    unsigned size = static_cast<unsigned>(m_byteCount);
    char* data = new char[size];
    if (m_byteCount != 0) {
        unsigned long long pos = 0;
        while (pos < m_byteCount) {
            unsigned count = static_cast<unsigned>(m_byteCount) - static_cast<unsigned>(pos);
            AudioFileReadBytes(m_file, 0, static_cast<long long>(pos), &count, data + static_cast<unsigned>(pos));
            pos += count;
        }
    }
    m_audioData = data;
    close();
    m_contentType = new String("audio/x-wav");
}

void SharedALBuffer::realize() {
    if (isRealized())
        return;
    AudioLock lock;
    loadAudioData();
    alGenBuffers(1, &m_buffer);
    int rate = static_cast<int>(m_format.mSampleRate);
    alBufferDataStatic(m_buffer, m_alFormat, getAudioData(), static_cast<int>(m_byteCount), rate);
}

SharedALBuffer::~SharedALBuffer() {
    invalidate();
    if (m_audioData != 0)
        delete[] static_cast<char*>(m_audioData);
    close();
}

SharedALBuffer::SharedALBuffer(OpaqueAudioFileID* file)
    : m_audioData(0), m_file(file), m_byteCount(0), m_alFormat(0), m_buffer(0) {
    memset(&m_format, 0, sizeof(m_format));
    m_byteCount = 0;
    m_alFormat = 0;
    m_buffer = 0;
    unsigned int size = 40;
    AudioFileGetProperty(m_file, kAudioFilePropertyDataFormat, &size, &m_format);
    size = 8;
    AudioFileGetProperty(m_file, kAudioFilePropertyAudioDataByteCount, &size, &m_byteCount);
    m_alFormat = AL_FORMAT_STEREO16;
    if (m_format.mChannelsPerFrame == 2) {
        if (m_format.mBitsPerChannel == 16)
            m_alFormat = AL_FORMAT_STEREO16;
        else if (m_format.mBitsPerChannel == 8)
            m_alFormat = AL_FORMAT_STEREO8;
    } else if (m_format.mChannelsPerFrame == 1) {
        if (m_format.mBitsPerChannel == 16)
            m_alFormat = AL_FORMAT_MONO16;
        else if (m_format.mBitsPerChannel == 8)
            m_alFormat = AL_FORMAT_MONO8;
    }
}

bool SharedALBuffer::isAudioDataLoaded() const {
    return m_audioData != 0;
}

bool SharedALBuffer::isRealized() const {
    return m_buffer != 0;
}

unsigned int SharedALBuffer::getSharedBuffer() const {
    return m_buffer;
}

String* SharedALBuffer::getContentType() {
    return m_contentType;
}

}
