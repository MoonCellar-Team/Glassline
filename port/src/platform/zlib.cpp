#include <stdint.h>
#include <zlib.h>

#include <mutex>
#include <unordered_map>

namespace {

struct GameStream {
    unsigned char* next_in;
    uint32_t avail_in;
    uint32_t total_in;
    unsigned char* next_out;
    uint32_t avail_out;
    uint32_t total_out;
    uint32_t msg, state, zalloc, zfree, opaque;
    int32_t data_type;
    uint32_t adler, reserved;
};

std::mutex s_mutex;
std::unordered_map<void*, z_stream*> s_streams;

z_stream* find(void* stream) {
    std::lock_guard<std::mutex> l(s_mutex);
    auto it = s_streams.find(stream);
    return it == s_streams.end() ? nullptr : it->second;
}

}

extern "C" {

int inflateInit2__port(void* stream, int windowBits, const char*, int) {
    GameStream* g = static_cast<GameStream*>(stream);
    z_stream* z = new z_stream();
    z->next_in = g->next_in;
    z->avail_in = g->avail_in;
    int r = inflateInit2(z, windowBits);
    if (r != Z_OK) {
        delete z;
        return r;
    }
    g->total_in = g->total_out = 0;
    g->msg = 0;
    std::lock_guard<std::mutex> l(s_mutex);
    s_streams[stream] = z;
    return r;
}

int inflate_port(void* stream, int flush) {
    GameStream* g = static_cast<GameStream*>(stream);
    z_stream* z = find(stream);
    if (!z) return Z_STREAM_ERROR;
    z->next_in = g->next_in;
    z->avail_in = g->avail_in;
    z->next_out = g->next_out;
    z->avail_out = g->avail_out;
    int r = inflate(z, flush);
    g->total_in += g->avail_in - z->avail_in;
    g->total_out += g->avail_out - z->avail_out;
    g->next_in = z->next_in;
    g->avail_in = z->avail_in;
    g->next_out = z->next_out;
    g->avail_out = z->avail_out;
    g->data_type = z->data_type;
    g->adler = static_cast<uint32_t>(z->adler);
    return r;
}

int inflateEnd_port(void* stream) {
    z_stream* z;
    {
        std::lock_guard<std::mutex> l(s_mutex);
        auto it = s_streams.find(stream);
        if (it == s_streams.end()) return Z_STREAM_ERROR;
        z = it->second;
        s_streams.erase(it);
    }
    int r = inflateEnd(z);
    delete z;
    return r;
}

}
