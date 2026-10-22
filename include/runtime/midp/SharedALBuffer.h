#pragma once

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "sdk/AudioToolbox.h"

struct OpaqueAudioFileID;

namespace midp {

class String;

class SharedALBuffer : public ReferenceCounted {
public:
    explicit SharedALBuffer(OpaqueAudioFileID* file);
    virtual ~SharedALBuffer();

    const void* getAudioData() const;
    void close();
    void invalidate();
    void loadAudioData();
    void realize();
    bool isAudioDataLoaded() const;
    bool isRealized() const;
    unsigned int getSharedBuffer() const;
    String* getContentType();

    void* m_audioData;
    OpaqueAudioFileID* m_file;
    AudioStreamBasicDescription m_format;
    unsigned long long m_byteCount;
    int m_alFormat;
    unsigned int m_buffer;
    ReferenceCountedPointer<String> m_contentType;
};

}
