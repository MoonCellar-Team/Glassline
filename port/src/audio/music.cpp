#include "music.h"

#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <AL/al.h>
#include <AL/alc.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <random>
#include <thread>

#include "../platform/settings.h"
#include "openal.h"

#ifdef MEDECOMP_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}
#endif

namespace port {
namespace audio {

namespace settings = port::platform::settings;

bool musicSetting() { return settings::get("music", "off") == "on"; }
void setMusicSetting(bool on) { settings::set("music", on ? "on" : "off"); }
std::string musicFolderSetting() { return settings::get("music_folder", ""); }
void setMusicFolderSetting(const std::string& folder) { settings::set("music_folder", folder); }
bool musicShuffleSetting() { return settings::get("music_shuffle", "on") != "off"; }
void setMusicShuffleSetting(bool on) { settings::set("music_shuffle", on ? "on" : "off"); }

namespace {

int percent(const std::string& value, int fallback) {
    if (value.empty()) return fallback;
    int v = atoi(value.c_str());
    return v < 0 ? 0 : v > 100 ? 100 : v;
}

}

int musicVolumeSetting() { return percent(settings::get("music_volume", "100"), 100); }
void setMusicVolumeSetting(int v) { settings::set("music_volume", std::to_string(v < 0 ? 0 : v > 100 ? 100 : v)); }
int volumeSetting() { return percent(settings::get("volume", "100"), 100); }

bool backgroundPauseSetting() { return settings::get("background_audio", "pause") != "play"; }

void setBackgroundPauseSetting(bool pause) { settings::set("background_audio", pause ? "pause" : "play"); }
void setVolumeSetting(int v) { settings::set("volume", std::to_string(v < 0 ? 0 : v > 100 ? 100 : v)); }

namespace {

bool audioFile(const char* name) {
    static const char* const kExt[] = {".mp3", ".ogg", ".oga", ".opus", ".flac", ".wav", ".m4a",
                                       ".aac", ".wma", ".ape", ".wv",  ".mka",  ".caf"};
    const char* dot = strrchr(name, '.');
    if (!dot) return false;
    for (const char* ext : kExt)
        if (strcasecmp(dot, ext) == 0) return true;
    return false;
}

void scan(const std::string& dir, int depth, std::vector<std::string>* out) {
    if (depth > 8 || out->size() >= 10000) return;
    DIR* d = opendir(dir.c_str());
    if (!d) return;
    while (dirent* e = readdir(d)) {
        if (e->d_name[0] == '.') continue;
        std::string path = dir + "/" + e->d_name;
        struct stat st;
        if (stat(path.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode))
            scan(path, depth + 1, out);
        else if (S_ISREG(st.st_mode) && audioFile(e->d_name))
            out->push_back(path);
    }
    closedir(d);
}

}

std::vector<std::string> musicTracks(const std::string& folder) {
    std::vector<std::string> tracks;
    if (folder.empty()) return tracks;
    std::string dir = folder;
    while (dir.size() > 1 && dir.back() == '/') dir.pop_back();
    scan(dir, 0, &tracks);
    std::sort(tracks.begin(), tracks.end());
    return tracks;
}

#ifndef MEDECOMP_FFMPEG

bool musicSupported() { return false; }
bool ownMusicPlaying() { return false; }

#else

bool musicSupported() { return true; }

namespace {

const int kRate = 44100;
const int kChunk = 8192;
const int kBuffers = 4;

class Decoder {
public:
    ~Decoder() { close(); }

    bool open(const std::string& path) {
        close();
        if (avformat_open_input(&m_format, path.c_str(), nullptr, nullptr) < 0) {
            m_format = nullptr;
            return false;
        }
        if (avformat_find_stream_info(m_format, nullptr) < 0) return false;
        const AVCodec* decoder = nullptr;
        m_stream = av_find_best_stream(m_format, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
        if (m_stream < 0 || !decoder) return false;
        m_codec = avcodec_alloc_context3(decoder);
        if (!m_codec || avcodec_parameters_to_context(m_codec, m_format->streams[m_stream]->codecpar) < 0 ||
            avcodec_open2(m_codec, decoder, nullptr) < 0)
            return false;
        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout in = m_codec->ch_layout;
        if (in.nb_channels <= 0) av_channel_layout_default(&in, 2);
        if (swr_alloc_set_opts2(&m_swr, &stereo, AV_SAMPLE_FMT_S16, kRate, &in, m_codec->sample_fmt,
                                m_codec->sample_rate, 0, nullptr) < 0 ||
            swr_init(m_swr) < 0)
            return false;
        m_packet = av_packet_alloc();
        m_frame = av_frame_alloc();
        m_eof = false;
        m_flushed = false;
        return m_packet && m_frame;
    }

    void close() {
        av_frame_free(&m_frame);
        av_packet_free(&m_packet);
        swr_free(&m_swr);
        avcodec_free_context(&m_codec);
        if (m_format) avformat_close_input(&m_format);
        m_pending.clear();
        m_pos = 0;
    }

    int read(int16_t* out, int frames) {
        int done = 0;
        while (done < frames) {
            if (m_pos < m_pending.size()) {
                size_t n = std::min(size_t(frames - done) * 2, m_pending.size() - m_pos);
                memcpy(out + done * 2, m_pending.data() + m_pos, n * sizeof(int16_t));
                m_pos += n;
                done += int(n / 2);
                continue;
            }
            if (!decodeMore()) break;
        }
        return done;
    }

private:
    bool decodeMore() {
        m_pending.clear();
        m_pos = 0;
        while (true) {
            int r = avcodec_receive_frame(m_codec, m_frame);
            if (r == 0) {
                convert(m_frame->extended_data, m_frame->nb_samples);
                av_frame_unref(m_frame);
                if (!m_pending.empty()) return true;
                continue;
            }
            if (r == AVERROR_EOF) {
                if (!m_flushed) {
                    m_flushed = true;
                    convert(nullptr, 0);
                    if (!m_pending.empty()) return true;
                }
                return false;
            }
            if (m_eof) return false;
            r = av_read_frame(m_format, m_packet);
            if (r < 0) {
                m_eof = true;
                avcodec_send_packet(m_codec, nullptr);
                continue;
            }
            if (m_packet->stream_index == m_stream) avcodec_send_packet(m_codec, m_packet);
            av_packet_unref(m_packet);
        }
    }

    void convert(uint8_t** in, int count) {
        int room = swr_get_out_samples(m_swr, count);
        if (room <= 0) return;
        m_pending.resize(size_t(room) * 2);
        uint8_t* outp = reinterpret_cast<uint8_t*>(m_pending.data());
        int got = swr_convert(m_swr, &outp, room, const_cast<const uint8_t**>(in), count);
        m_pending.resize(got > 0 ? size_t(got) * 2 : 0);
    }

    AVFormatContext* m_format = nullptr;
    AVCodecContext* m_codec = nullptr;
    SwrContext* m_swr = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame* m_frame = nullptr;
    int m_stream = -1;
    bool m_eof = false, m_flushed = false;
    std::vector<int16_t> m_pending;
    size_t m_pos = 0;
};

std::string fileName(const std::string& path) {
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

using SetThreadContext = ALCboolean (*)(ALCcontext*);

std::atomic<bool> s_playing(false);

void player(std::vector<std::string> tracks, bool shuffle, float gain) {
    ALCdevice* device = alcOpenDevice(nullptr);
    const ALCint attrs[] = {ALC_FREQUENCY, kRate, 0};
    ALCcontext* context = device ? alcCreateContext(device, attrs) : nullptr;
    SetThreadContext setThread =
        reinterpret_cast<SetThreadContext>(alcGetProcAddress(nullptr, "alcSetThreadContext"));
    if (!context || !setThread || !setThread(context)) {
        log("своя музыка: не открыть звук OpenAL");
        s_playing = false;
        if (context) alcDestroyContext(context);
        if (device) alcCloseDevice(device);
        return;
    }
    ALuint source = 0, buffers[kBuffers];
    alGenSources(1, &source);
    alGenBuffers(kBuffers, buffers);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(source, AL_POSITION, 0, 0, 0);
    alSourcef(source, AL_GAIN, gain);
    std::vector<ALuint> free(buffers, buffers + kBuffers);

    std::vector<size_t> order(tracks.size());
    for (size_t i = 0; i < order.size(); i++) order[i] = i;
    std::mt19937 random(unsigned(time(nullptr)));
    size_t next = order.size();
    int failedInRow = 0;
    Decoder decoder;
    bool open = false;
    std::vector<int16_t> chunk(size_t(kChunk) * 2);

    while (true) {
        ALint processed = 0;
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
        while (processed-- > 0) {
            ALuint b;
            alSourceUnqueueBuffers(source, 1, &b);
            free.push_back(b);
        }
        while (!free.empty()) {
            if (!open) {
                if (next >= order.size()) {
                    if (shuffle) std::shuffle(order.begin(), order.end(), random);
                    next = 0;
                }
                const std::string& path = tracks[order[next++]];
                open = decoder.open(path);
                if (!open) {
                    log("своя музыка: не читается %s", fileName(path).c_str());
                    if (++failedInRow >= int(tracks.size())) {
                        log("своя музыка: ни один трек не читается — играет музыка игры");
                        s_playing = false;
                        return;
                    }
                    continue;
                }
                failedInRow = 0;
                log("своя музыка: %s", fileName(path).c_str());
            }
            int frames = decoder.read(chunk.data(), kChunk);
            if (frames == 0) {
                open = false;
                continue;
            }
            ALuint b = free.back();
            free.pop_back();
            alBufferData(b, AL_FORMAT_STEREO16, chunk.data(), frames * 4, kRate);
            alSourceQueueBuffers(source, 1, &b);
        }
        ALint state = 0;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) alSourcePlay(source);
        usleep(20000);
    }
}

std::once_flag s_start;

}

bool ownMusicPlaying() {
    std::call_once(s_start, [] {
        if (!musicSetting()) return;
        std::vector<std::string> tracks = musicTracks(musicFolderSetting());
        log("своя музыка: %s — треков %zu", musicFolderSetting().c_str(), tracks.size());
        if (tracks.empty()) return;
        s_playing = true;
        std::thread(player, std::move(tracks), musicShuffleSetting(), musicVolumeSetting() / 100.0f).detach();
    });
    return s_playing;
}

#endif

}
}

extern "C" int medecomp_background_audio(void) { return port::audio::backgroundPauseSetting() ? 0 : 1; }
