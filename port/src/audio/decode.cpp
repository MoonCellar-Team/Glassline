#include "decode.h"

#include <string.h>

#if MEDECOMP_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/log.h>
#include <libswresample/swresample.h>
}

namespace {

struct Reader {
    const std::vector<unsigned char>* data;
    size_t pos;
};

int readPacket(void* opaque, uint8_t* buf, int size) {
    Reader* r = static_cast<Reader*>(opaque);
    size_t left = r->data->size() - r->pos;
    if (left == 0) return AVERROR_EOF;
    size_t n = left < size_t(size) ? left : size_t(size);
    memcpy(buf, r->data->data() + r->pos, n);
    r->pos += n;
    return int(n);
}

int64_t seekPacket(void* opaque, int64_t offset, int whence) {
    Reader* r = static_cast<Reader*>(opaque);
    int64_t size = int64_t(r->data->size());
    if (whence == AVSEEK_SIZE) return size;
    whence &= ~AVSEEK_FORCE;
    int64_t base = whence == SEEK_SET ? 0 : whence == SEEK_CUR ? int64_t(r->pos) : size;
    int64_t pos = base + offset;
    if (pos < 0 || pos > size) return -1;
    r->pos = size_t(pos);
    return pos;
}

}
#endif

namespace port {
namespace audio {

#if !MEDECOMP_FFMPEG
bool decodeToPcm16(const std::vector<unsigned char>&, std::vector<unsigned char>*, int*, int*) { return false; }
#else
bool decodeToPcm16(const std::vector<unsigned char>& file, std::vector<unsigned char>* pcm, int* rate,
                   int* channels) {
    pcm->clear();
    av_log_set_level(AV_LOG_ERROR);
    Reader reader = {&file, 0};
    const int kIoSize = 65536;
    unsigned char* ioBuffer = static_cast<unsigned char*>(av_malloc(kIoSize));
    AVIOContext* io = avio_alloc_context(ioBuffer, kIoSize, 0, &reader, readPacket, nullptr, seekPacket);
    AVFormatContext* format = avformat_alloc_context();
    AVCodecContext* codec = nullptr;
    SwrContext* swr = nullptr;
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVChannelLayout layout;
    memset(&layout, 0, sizeof layout);
    bool ok = false;
    int stream = -1;
    const AVCodec* decoder = nullptr;
    format->pb = io;
    format->flags |= AVFMT_FLAG_CUSTOM_IO;
    if (avformat_open_input(&format, nullptr, nullptr, nullptr) < 0) {
        format = nullptr;
        goto done;
    }
    if (avformat_find_stream_info(format, nullptr) < 0) goto done;
    stream = av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
    if (stream < 0 || !decoder) goto done;
    codec = avcodec_alloc_context3(decoder);
    if (!codec || avcodec_parameters_to_context(codec, format->streams[stream]->codecpar) < 0 ||
        avcodec_open2(codec, decoder, nullptr) < 0)
        goto done;
    if (av_channel_layout_copy(&layout, &codec->ch_layout) < 0) goto done;
    if (layout.order == AV_CHANNEL_ORDER_UNSPEC) av_channel_layout_default(&layout, layout.nb_channels);
    *rate = codec->sample_rate;
    *channels = layout.nb_channels;
    if (*rate <= 0 || *channels <= 0 || *channels > 2) goto done;
    if (swr_alloc_set_opts2(&swr, &layout, AV_SAMPLE_FMT_S16, *rate, &layout, codec->sample_fmt, *rate, 0,
                            nullptr) < 0 ||
        swr_init(swr) < 0)
        goto done;
    {
        size_t frameBytes = size_t(*channels) * 2;
        auto convert = [&](const uint8_t** in, int count) {
            int room = swr_get_out_samples(swr, count);
            if (room <= 0) return;
            size_t old = pcm->size();
            pcm->resize(old + size_t(room) * frameBytes);
            uint8_t* out = pcm->data() + old;
            int got = swr_convert(swr, &out, room, in, count);
            pcm->resize(old + size_t(got > 0 ? got : 0) * frameBytes);
        };
        auto drain = [&]() {
            while (avcodec_receive_frame(codec, frame) == 0) {
                convert(const_cast<const uint8_t**>(frame->extended_data), frame->nb_samples);
                av_frame_unref(frame);
            }
        };
        while (av_read_frame(format, packet) >= 0) {
            if (packet->stream_index == stream && avcodec_send_packet(codec, packet) == 0) drain();
            av_packet_unref(packet);
        }
        avcodec_send_packet(codec, nullptr);
        drain();
        convert(nullptr, 0);
    }
    ok = !pcm->empty();
done:
    av_channel_layout_uninit(&layout);
    swr_free(&swr);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec);
    if (format) avformat_close_input(&format);
    if (io) {
        av_freep(&io->buffer);
        avio_context_free(&io);
    }
    return ok;
}
#endif

}
}
