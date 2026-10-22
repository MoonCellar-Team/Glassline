#pragma once

extern "C" {

struct OpaqueAudioQueue;
struct AudioQueueBuffer;
struct OpaqueAudioFileID;
struct __CFURL;

int AudioFileOpenURL(const __CFURL* url, int permissions, unsigned int fileTypeHint,
                     OpaqueAudioFileID** outFile);
int AudioFileGetProperty(OpaqueAudioFileID* file, unsigned int property, unsigned int* dataSize,
                         void* data);

struct AudioStreamBasicDescription {
    double mSampleRate;
    unsigned int mFormatID;
    unsigned int mFormatFlags;
    unsigned int mBytesPerPacket;
    unsigned int mFramesPerPacket;
    unsigned int mBytesPerFrame;
    unsigned int mChannelsPerFrame;
    unsigned int mBitsPerChannel;
    unsigned int mReserved;
};

}
