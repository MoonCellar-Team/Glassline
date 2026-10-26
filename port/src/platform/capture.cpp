#include "capture.h"

#include <SDL.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "../audio/openal.h"
#include "game/AppEngine.h"
#include "game/GameObjectPlayer.h"
#include "game/SceneGame.h"
#include "input.h"
#include "port/platform.h"
#include "runtime/midp/System.h"

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

extern "C" void medecomp_quit_game(void);

namespace port {
namespace platform {
namespace capture {

namespace {

const int kRate = 48000;
const int kFps = 60;
const int kSamples = kRate / kFps;
const long long kSeed = 0x6c617373;

enum Mode { Off, Record, Replay };
Mode s_mode = Off;
bool s_video;
std::string s_videoPath, s_wavPath;

struct Pending {
    int kind, x, y, px, py, tap;
    bool mouse;
};
std::mutex s_pendingMu;
std::vector<Pending> s_pending;

struct Event {
    long long frame;
    char type;
    int kind, x, y, pointer, tap;
    float tilt;
};
std::vector<Event> s_events;
size_t s_next;
long long s_end = -1;

FILE* s_record;
FILE* s_trace;
long long s_frame = -1;
bool s_ended;
uint32_t s_lastTilt;
bool s_tiltLogged;
float s_replayTilt;
std::atomic<long long> s_seeds{0};

SDL_AudioDeviceID s_speakers;
bool s_speakersTried;

FILE* s_ffmpeg;
FILE* s_wav;
long long s_wavFrames;
int s_videoW, s_videoH;
std::vector<unsigned char> s_lastFrame;
bool s_frameTaken;
std::vector<float> s_mix(kSamples * 2);

uint32_t bits(float f) {
    uint32_t u;
    memcpy(&u, &f, sizeof u);
    return u;
}

std::string quoted(const std::string& s) {
#ifdef _WIN32
    return "\"" + s + "\"";
#else
    std::string out = "'";
    for (char c : s) out += c == '\'' ? std::string("'\\''") : std::string(1, c);
    return out + "'";
#endif
}

void put16(FILE* f, uint16_t v) { fwrite(&v, 2, 1, f); }
void put32(FILE* f, uint32_t v) { fwrite(&v, 4, 1, f); }

void wavHeader(FILE* f, uint32_t frames) {
    uint32_t data = frames * 8;
    fwrite("RIFF", 1, 4, f);
    put32(f, 36 + data);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16);
    put16(f, 3);
    put16(f, 2);
    put32(f, kRate);
    put32(f, kRate * 8);
    put16(f, 8);
    put16(f, 32);
    fwrite("data", 1, 4, f);
    put32(f, data);
}

bool loadReplay(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "medecomp: запись забега: нет файла %s\n", path);
        return false;
    }
    char line[256];
    long long last = 0;
    bool header = false;
    while (fgets(line, sizeof line, f)) {
        Event e{};
        char tilt[64];
        if (!header) {
            header = strncmp(line, "glassline-run 1", 15) == 0;
            if (!header) break;
            continue;
        }
        if (sscanf(line, "p %lld %d %d %d %d %d", &e.frame, &e.kind, &e.x, &e.y, &e.pointer, &e.tap) == 6) {
            e.type = 'p';
        } else if (sscanf(line, "t %lld %63s", &e.frame, tilt) == 2) {
            e.type = 't';
            e.tilt = strtof(tilt, nullptr);
        } else if (sscanf(line, "e %lld", &s_end) == 1) {
            continue;
        } else {
            continue;
        }
        last = e.frame;
        s_events.push_back(e);
    }
    fclose(f);
    if (!header) {
        fprintf(stderr, "medecomp: запись забега: %s — не запись (нет «glassline-run 1»)\n", path);
        return false;
    }
    if (s_end < 0) s_end = last + 5 * kFps;
    fprintf(stderr, "medecomp: запись забега: проигрыш %s — %zu событий, %lld кадров (%.1f с)\n", path,
            s_events.size(), s_end + 1, (s_end + 1) / double(kFps));
    return true;
}

void openVideo(int w, int h) {
    s_videoW = w;
    s_videoH = h;
    std::string cmd = "ffmpeg -hide_banner -loglevel error -y -f rawvideo -pix_fmt rgba -s " + std::to_string(w) + "x" +
                      std::to_string(h) + " -framerate " + std::to_string(kFps) +
                      " -i - -vf vflip -c:v libx264 -preset veryfast -crf 12 -pix_fmt yuv444p " + quoted(s_videoPath);
#ifdef _WIN32
    s_ffmpeg = popen(cmd.c_str(), "wb");
#else
    s_ffmpeg = popen(cmd.c_str(), "w");
#endif
    if (!s_ffmpeg) fprintf(stderr, "medecomp: запись забега: ffmpeg не запустить\n");
    else fprintf(stderr, "medecomp: запись забега: видео %dx%d — %s, звук — %s\n", w, h, s_videoPath.c_str(), s_wavPath.c_str());
}

void writeFrame(const unsigned char* rgba) {
    if (s_ffmpeg && fwrite(rgba, size_t(s_videoW) * s_videoH * 4, 1, s_ffmpeg) != 1) {
        fprintf(stderr, "medecomp: запись забега: ffmpeg не принял кадр — видео остановлено\n");
        pclose(s_ffmpeg);
        s_ffmpeg = nullptr;
    }
}

void closeVideo() {
    if (s_ffmpeg) {
        int code = pclose(s_ffmpeg);
        s_ffmpeg = nullptr;
        fprintf(stderr, code == 0 ? "medecomp: запись забега: видео готово — %s\n"
                                  : "medecomp: запись забега: ffmpeg закончил с ошибкой — %s\n",
                s_videoPath.c_str());
    }
    if (s_wav) {
        fseek(s_wav, 0, SEEK_SET);
        wavHeader(s_wav, uint32_t(s_wavFrames));
        fclose(s_wav);
        s_wav = nullptr;
    }
    s_video = false;
}

void playMix() {
    if (!s_speakersTried) {
        s_speakersTried = true;
        SDL_AudioSpec want{}, have{};
        want.freq = kRate;
        want.format = AUDIO_F32SYS;
        want.channels = 2;
        want.samples = 512;
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) s_speakers = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (s_speakers) {
            SDL_PauseAudioDevice(s_speakers, 0);
            fprintf(stderr, "medecomp: запись забега: звук — в колонки (SDL, %s, %d Гц)\n", SDL_GetCurrentAudioDriver(), have.freq);
        } else
            fprintf(stderr, "medecomp: запись забега: звук в колонки не вывести: %s\n", SDL_GetError());
    }
    if (!s_speakers) return;
    const Uint32 bytes = Uint32(s_mix.size() * sizeof(float));
    if (SDL_GetQueuedAudioSize(s_speakers) < Uint32(kRate * 8 * 0.15)) SDL_QueueAudio(s_speakers, s_mix.data(), bytes);
}

void traceFrame() {
    if (!s_trace) return;
    AppEngine* app = AppEngine::getCanvas();
    SceneGame* game = app ? app->getSceneGame() : nullptr;
    GameObjectPlayer* player = game ? game->getSinglePlayerObject() : nullptr;
    unsigned long long seed = app ? app->m_random.m_seed : 0;
    if (player)
        fprintf(s_trace, "%lld %a %a %a %d %llx\n", s_frame, player->m_position.x, player->m_position.y,
                player->m_position.z, int(player->getState()), seed);
    else
        fprintf(s_trace, "%lld - %llx\n", s_frame, seed);
}

}

void start() {
    const char* record = getenv("MEDECOMP_RECORD");
    const char* replay = getenv("MEDECOMP_REPLAY");
    if (replay && *replay) {
        if (!loadReplay(replay)) exit(1);
        s_mode = Replay;
    } else if (record && *record) {
        s_record = fopen(record, "w");
        if (!s_record) {
            fprintf(stderr, "medecomp: запись забега: не записать %s\n", record);
            exit(1);
        }
        fprintf(s_record, "glassline-run 1\n");
        fflush(s_record);
        s_mode = Record;
        fprintf(stderr, "medecomp: запись забега: ввод уровня — в %s\n", record);
    } else {
        return;
    }
    if (const char* trace = getenv("MEDECOMP_TRACE")) s_trace = fopen(trace, "w");
    port::audio::useLoopback(kRate);
    const char* video = getenv("MEDECOMP_VIDEO");
    if (s_mode == Replay && video && *video) {
        s_videoPath = video;
        size_t dot = s_videoPath.find_last_of('.');
        size_t slash = s_videoPath.find_last_of("/\\");
        s_wavPath = (dot != std::string::npos && (slash == std::string::npos || dot > slash) ? s_videoPath.substr(0, dot)
                                                                                             : s_videoPath) +
                    ".wav";
        s_wav = fopen(s_wavPath.c_str(), "wb");
        if (!s_wav) {
            fprintf(stderr, "medecomp: запись забега: не записать %s\n", s_wavPath.c_str());
            exit(1);
        }
        wavHeader(s_wav, 0);
        s_video = true;
#ifndef _WIN32
        signal(SIGPIPE, SIG_IGN);
#endif
    }
}

bool active() { return s_mode != Off; }

bool replaying() { return s_mode == Replay; }

void postPointer(int kind, int x, int y, int px, int py, int tap, bool mouse) {
    if (s_mode != Record) return;
    std::lock_guard<std::mutex> lock(s_pendingMu);
    s_pending.push_back({kind, x, y, px, py, tap, mouse});
}

int frameBegin() {
    s_frameTaken = false;
    if (s_mode == Record) {
        std::vector<Pending> pending;
        {
            std::lock_guard<std::mutex> lock(s_pendingMu);
            pending.swap(s_pending);
        }
        for (const Pending& p : pending) {
            int pointer = pointerCall(p.kind, p.x, p.y, p.px, p.py, p.tap, p.mouse);
            if (s_frame >= 0 && !s_ended)
                fprintf(s_record, "p %lld %d %d %d %d %d\n", s_frame, p.kind, p.x, p.y, pointer, p.tap);
        }
        float t = tilt();
        if (s_frame >= 0 && !s_ended && (!s_tiltLogged || bits(t) != s_lastTilt)) {
            fprintf(s_record, "t %lld %a\n", s_frame, t);
            s_lastTilt = bits(t);
            s_tiltLogged = true;
        }
        accelerate(t);
    } else if (s_mode == Replay) {
        if (s_frame >= 0) {
            for (; s_next < s_events.size() && s_events[s_next].frame <= s_frame; s_next++) {
                const Event& e = s_events[s_next];
                if (e.type == 'p')
                    pointerApply(e.kind, e.x, e.y, e.pointer, e.tap);
                else
                    s_replayTilt = e.tilt;
            }
        }
        accelerate(s_frame >= 0 ? s_replayTilt : 0.0f);
    }
    if (s_frame < 0) return 17;
    return int((s_frame + 1) * 1000 / kFps - s_frame * 1000 / kFps);
}

bool wantsVideoFrame() { return s_video && s_frame >= 0 && !s_frameTaken; }

void videoFrame(const unsigned char* rgba, int w, int h) {
    s_frameTaken = true;
    if (!s_ffmpeg && s_videoW == 0) openVideo(w, h);
    if (w != s_videoW || h != s_videoH) {
        fprintf(stderr, "medecomp: запись забега: кадр %dx%d вместо %dx%d — повтор прошлого\n", w, h, s_videoW, s_videoH);
        return;
    }
    s_lastFrame.assign(rgba, rgba + size_t(w) * h * 4);
    writeFrame(rgba);
}

void frameEnd() {
    port::audio::renderLoopback(s_mix.data(), kSamples);
    if (s_wavPath.empty()) playMix();
    if (s_video) {
        if (s_frame >= 0 && s_wav) {
            fwrite(s_mix.data(), sizeof(float), s_mix.size(), s_wav);
            s_wavFrames += kSamples;
        }
        if (s_frame >= 0 && (!s_frameTaken || s_lastFrame.size() != size_t(s_videoW) * s_videoH * 4) &&
            !s_lastFrame.empty())
            writeFrame(s_lastFrame.data());
    }
    if (s_frame < 0 || s_ended) return;
    traceFrame();
    if (s_mode == Replay && s_frame >= s_end && !s_ended) {
        s_ended = true;
        if (s_video) closeVideo();
        if (s_trace) fflush(s_trace);
        fprintf(stderr, "medecomp: запись забега: проиграна до конца (%lld кадров) — выход\n", s_frame + 1);
        medecomp_quit_game();
        return;
    }
    if (s_mode == Record && s_frame % kFps == 0) fflush(s_record);
    s_frame++;
}

void finish() {
    if (s_mode == Record && s_record) {
        long long last = s_frame > 0 ? s_frame - 1 : 0;
        fprintf(s_record, "e %lld\n", last);
        fclose(s_record);
        s_record = nullptr;
        fprintf(stderr, "medecomp: запись забега: %lld кадров (%.1f с)\n", last + 1, (last + 1) / double(kFps));
    }
    if (s_video) closeVideo();
    if (s_trace) {
        fclose(s_trace);
        s_trace = nullptr;
    }
    if (s_speakers) {
        SDL_CloseAudioDevice(s_speakers);
        s_speakers = 0;
    }
}

}
}
}

extern "C" void medecomp_capture_level_start(void) {
    using namespace port::platform::capture;
    if (!active() || s_frame >= 0) return;
    s_frame = 0;
    s_seeds = 0;
    if (AppEngine* app = AppEngine::getCanvas()) app->m_random.setSeed(kSeed);
    fprintf(stderr, "medecomp: запись забега: уровень загружен — кадр 0\n");
}

extern "C" int medecomp_capture_autostart(void) { return port::platform::capture::replaying(); }

extern "C" long long medecomp_random_seed(void) {
    using namespace port::platform::capture;
    if (!active()) return midp::System::currentTimeMillis();
    return kSeed * 31 + 0x9e3779b9LL * (s_seeds.fetch_add(1) + 1);
}
