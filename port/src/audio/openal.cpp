#define _GNU_SOURCE
#ifndef __EMSCRIPTEN__
#include <dlfcn.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "../platform/settings.h"
#include "music.h"
#include "openal.h"
#include "port/files.h"

namespace {

using OpenDevice = void* (*)(const char*);
using CloseDevice = int (*)(void*);
using CreateContext = void* (*)(void*, const int*);
using GetProc = void* (*)(void*, const char*);
using DeviceCall = void (*)(void*);
using GetIntegerv = void (*)(void*, int, int, int*);

const int kAlcHrtfSoft = 0x1992;
const int kAlcMonoSources = 0x1010;
const int kAlcStereoSources = 0x1011;
const int kAlcFrequency = 0x1007;
const int kAlcFormatChannelsSoft = 0x1990;
const int kAlcFormatTypeSoft = 0x1991;
const int kAlcStereoSoft = 0x1501;
const int kAlcFloatSoft = 0x1406;

const int kSources = 1024;

#ifdef __EMSCRIPTEN__
#define HOOK(name) __wrap_##name
}
extern "C" {
void* __real_alGetProcAddress(const char*);
void* __real_alcOpenDevice(const char*);
void __real_alGenSources(int, unsigned*);
void __real_alSourcePlay(unsigned);
void __real_alDeleteSources(int, const unsigned*);
void __real_alcDestroyContext(void*);
int __real_alcCloseDevice(void*);
void* __real_alcCreateContext(void*, const int*);
void* alcGetThreadContext();
void* alcGetCurrentContext();
void* alcGetProcAddress(void*, const char*);
int alcSetThreadContext(void*);
void alListenerf(int, float);
void alcGetIntegerv(void*, int, int, int*);
void alBufferData(unsigned, int, const void*, int, int);
}
namespace {

void* nextAddress(const char* name) {
    static const std::map<std::string, void*> table = {
        {"alGetProcAddress", reinterpret_cast<void*>(&__real_alGetProcAddress)},
        {"alcOpenDevice", reinterpret_cast<void*>(&__real_alcOpenDevice)},
        {"alGenSources", reinterpret_cast<void*>(&__real_alGenSources)},
        {"alSourcePlay", reinterpret_cast<void*>(&__real_alSourcePlay)},
        {"alDeleteSources", reinterpret_cast<void*>(&__real_alDeleteSources)},
        {"alcDestroyContext", reinterpret_cast<void*>(&__real_alcDestroyContext)},
        {"alcCloseDevice", reinterpret_cast<void*>(&__real_alcCloseDevice)},
        {"alcCreateContext", reinterpret_cast<void*>(&__real_alcCreateContext)},
        {"alcGetThreadContext", reinterpret_cast<void*>(&alcGetThreadContext)},
        {"alcGetCurrentContext", reinterpret_cast<void*>(&alcGetCurrentContext)},
        {"alcGetProcAddress", reinterpret_cast<void*>(&alcGetProcAddress)},
        {"alcSetThreadContext", reinterpret_cast<void*>(&alcSetThreadContext)},
        {"alListenerf", reinterpret_cast<void*>(&alListenerf)},
        {"alcGetIntegerv", reinterpret_cast<void*>(&alcGetIntegerv)},
        {"alBufferData", reinterpret_cast<void*>(&alBufferData)},
    };
    auto it = table.find(name);
    return it == table.end() ? nullptr : it->second;
}
#else
#define HOOK(name) name

void* nextAddress(const char* name) { return dlsym(RTLD_NEXT, name); }
#endif

template <class F>
F next(const char* name) {
    return reinterpret_cast<F>(nextAddress(name));
}

std::mutex s_mu;
std::vector<void*> s_devices;
bool s_paused;
int s_loopRate;
std::vector<void*> s_loopDevices;
int s_shared;
int s_sharedContexts;

std::mutex s_sourcesMu;
std::map<void*, int> s_sources;

void* currentContext() {
    using GetContext = void* (*)();
    static GetContext thread = next<GetContext>("alcGetThreadContext");
    static GetContext current = next<GetContext>("alcGetCurrentContext");
    void* context = thread ? thread() : nullptr;
    return context ? context : (current ? current() : nullptr);
}

void setDevicePaused(void* device, bool paused) {
    GetProc getProc = next<GetProc>("alcGetProcAddress");
    DeviceCall fn = getProc ? reinterpret_cast<DeviceCall>(getProc(device, paused ? "alcDevicePauseSOFT"
                                                                                   : "alcDeviceResumeSOFT"))
                            : nullptr;
    if (fn) fn(device);
}

using BufferData = void (*)(unsigned int, int, const void*, int, int);

void bufferDataStatic(unsigned int buffer, int format, const void* data, int size, int freq) {
    BufferData fn = next<BufferData>("alBufferData");
    if (fn != 0)
        fn(buffer, format, data, size, freq);
}

}

extern "C" void* HOOK(alGetProcAddress)(const char* name) {
    if (name != 0 && strcmp(name, "alBufferDataStatic") == 0)
        return reinterpret_cast<void*>(&bufferDataStatic);
    if (name != 0 && strcmp(name, "alcMacOSXMixerOutputRate") == 0)
        return 0;
#ifdef __EMSCRIPTEN__
    return __real_alGetProcAddress(name);
#else
    return dlsym(RTLD_NEXT, name);
#endif
}

extern "C" void* HOOK(alcOpenDevice)(const char* name) {
    if (s_loopRate) {
        GetProc getProc = next<GetProc>("alcGetProcAddress");
        OpenDevice open = getProc ? reinterpret_cast<OpenDevice>(getProc(nullptr, "alcLoopbackOpenDeviceSOFT")) : nullptr;
        void* device = open ? open(nullptr) : nullptr;
        if (!device) {
            fprintf(stderr, "medecomp: звук: loopback OpenAL (ALC_SOFT_loopback) не открыть — тишина\n");
            return nullptr;
        }
        std::lock_guard<std::mutex> lock(s_mu);
        s_loopDevices.push_back(device);
        return device;
    }
#ifdef __EMSCRIPTEN__
    {
        std::lock_guard<std::mutex> lock(s_mu);
        if (!s_devices.empty()) {
            s_shared++;
            return s_devices.front();
        }
    }
#endif
    void* device = next<OpenDevice>("alcOpenDevice")(name);
    if (device) {
        std::lock_guard<std::mutex> lock(s_mu);
        s_devices.push_back(device);
        if (s_paused) setDevicePaused(device, true);
    }
    return device;
}

extern "C" void HOOK(alGenSources)(int n, unsigned* ids) {
    using GenSources = void (*)(int, unsigned*);
    static GenSources gen = next<GenSources>("alGenSources");
    for (int i = 0; i < n; i++) ids[i] = 0;
    gen(n, ids);
    int made = 0;
    for (int i = 0; i < n; i++) made += ids[i] != 0;
    void* context = currentContext();
    std::lock_guard<std::mutex> lock(s_sourcesMu);
    int& count = s_sources[context];
    int before = count;
    count += made;
    if (made < n) {
        static bool told;
        if (!told) fprintf(stderr, "medecomp: OpenAL не дал источник (%s, у контекста их %d)\n",
                           context ? "предел контекста" : "нет текущего контекста", count);
        told = true;
        port::audio::log("звук: OpenAL не дал источник — %s, у контекста их %d",
                         context ? "предел контекста" : "нет текущего контекста", count);
    } else if (count / 64 != before / 64) {
        port::audio::log("звук: источников у контекста — %d", count);
    }
}

extern "C" void HOOK(alSourcePlay)(unsigned source) {
    using SourcePlay = void (*)(unsigned);
    static SourcePlay play = next<SourcePlay>("alSourcePlay");
    if (!currentContext()) port::audio::log("звук: alSourcePlay(%u) без текущего контекста", source);
    play(source);
}

extern "C" void medecomp_audio_silent(unsigned source, int state) {
    const char* why = source == 0 ? "без источника" : state == 0x1011 ? "источник не запущен" : "источник не отвечает";
    static bool told;
    if (!told) fprintf(stderr, "medecomp: звук: копия события молчала (%s) — освобождена\n", why);
    told = true;
    port::audio::log("звук: копия события молчала (%s, источник %u) — освобождена", why, source);
}

extern "C" void HOOK(alDeleteSources)(int n, const unsigned* ids) {
    using DeleteSources = void (*)(int, const unsigned*);
    static DeleteSources del = next<DeleteSources>("alDeleteSources");
    del(n, ids);
    std::lock_guard<std::mutex> lock(s_sourcesMu);
    s_sources[currentContext()] -= n;
}

extern "C" void HOOK(alcDestroyContext)(void* context) {
#ifdef __EMSCRIPTEN__
    {
        std::lock_guard<std::mutex> lock(s_mu);
        if (s_sharedContexts > 0) s_sharedContexts--;
    }
#endif
    using DestroyContext = void (*)(void*);
    next<DestroyContext>("alcDestroyContext")(context);
    std::lock_guard<std::mutex> lock(s_sourcesMu);
    s_sources.erase(context);
}

extern "C" int HOOK(alcCloseDevice)(void* device) {
    {
        std::lock_guard<std::mutex> lock(s_mu);
#ifdef __EMSCRIPTEN__
        if (s_shared > 0 && !s_devices.empty() && device == s_devices.front()) {
            s_shared--;
            return 1;
        }
#endif
        s_devices.erase(std::remove(s_devices.begin(), s_devices.end(), device), s_devices.end());
        s_loopDevices.erase(std::remove(s_loopDevices.begin(), s_loopDevices.end(), device), s_loopDevices.end());
    }
    return next<CloseDevice>("alcCloseDevice")(device);
}

extern "C" void* HOOK(alcCreateContext)(void* device, const int* attrs) {
    std::string hrtf = port::platform::settings::get("hrtf", "auto");
    std::vector<int> list;
    bool sources = false;
    bool loopback;
    {
        std::lock_guard<std::mutex> lock(s_mu);
        loopback = std::find(s_loopDevices.begin(), s_loopDevices.end(), device) != s_loopDevices.end();
    }
    for (int i = 0; attrs && attrs[i] != 0; i += 2) {
        if (attrs[i] == kAlcHrtfSoft) continue;
        if (loopback && attrs[i] == kAlcFrequency) continue;
        sources = sources || attrs[i] == kAlcMonoSources;
        list.push_back(attrs[i]);
        list.push_back(attrs[i + 1]);
    }
    if (loopback) {
        const int format[] = {kAlcFormatChannelsSoft, kAlcStereoSoft, kAlcFormatTypeSoft, kAlcFloatSoft, kAlcFrequency, s_loopRate};
        list.insert(list.end(), format, format + 6);
    }
    if (!sources) {
        list.push_back(kAlcMonoSources);
        list.push_back(kSources);
    }
    if (hrtf == "on" || hrtf == "off") {
        list.push_back(kAlcHrtfSoft);
        list.push_back(hrtf == "on" ? 1 : 0);
    }
    list.push_back(0);
#ifdef __EMSCRIPTEN__
    {
        std::lock_guard<std::mutex> lock(s_mu);
        if (!loopback && s_sharedContexts++ > 0) list.assign(1, 0);
    }
#endif
    void* context = next<CreateContext>("alcCreateContext")(device, list.data());
    float volume = port::audio::volumeSetting() / 100.0f;
    if (context && volume < 0.999f) {
        using GetThread = void* (*)();
        using SetThread = int (*)(void*);
        using ListenerF = void (*)(int, float);
        GetThread getThread = next<GetThread>("alcGetThreadContext");
        SetThread setThread = next<SetThread>("alcSetThreadContext");
        ListenerF listenerf = next<ListenerF>("alListenerf");
        if (getThread && setThread && listenerf) {
            void* previous = getThread();
            setThread(context);
            listenerf(0x100A, volume);
            setThread(previous);
        }
    }
    if (context && port::audio::logOn()) {
        int status = -1, mono = 0, stereo = 0;
        if (GetIntegerv get = next<GetIntegerv>("alcGetIntegerv")) {
            get(device, kAlcHrtfSoft, 1, &status);
            get(device, kAlcMonoSources, 1, &mono);
            get(device, kAlcStereoSources, 1, &stereo);
        }
        port::audio::log("звук: контекст OpenAL, объёмный звук (HRTF) %s — настройка «%s», общая громкость %d %%, "
                         "источников до %d",
                         status == 1 ? "включён" : "выключен", hrtf.c_str(), int(volume * 100 + 0.5f), mono + stereo);
    }
    return context;
}

namespace port {
namespace audio {

void useLoopback(int rate) { s_loopRate = rate; }

void renderLoopback(float* out, int frames) {
    using Render = void (*)(void*, void*, int);
    static Render render = [] {
        GetProc getProc = next<GetProc>("alcGetProcAddress");
        return getProc ? reinterpret_cast<Render>(getProc(nullptr, "alcRenderSamplesSOFT")) : nullptr;
    }();
    static std::vector<float> one;
    std::fill(out, out + 2 * frames, 0.0f);
    if (!render) return;
    one.resize(size_t(2) * frames);
    std::lock_guard<std::mutex> lock(s_mu);
    for (void* device : s_loopDevices) {
        render(device, one.data(), frames);
        for (int i = 0; i < 2 * frames; i++) out[i] += one[i];
    }
}

void setSessionActive(bool active) {
    std::lock_guard<std::mutex> lock(s_mu);
    if (s_paused == !active) return;
    s_paused = !active;
    for (void* device : s_devices) setDevicePaused(device, s_paused);
    if (logOn()) log("звук: %s (устройств: %zu)", s_paused ? "пауза" : "снова играет", s_devices.size());
}

bool logOn() {
    static const bool on = getenv("MEDECOMP_AUDIO_LOG") != nullptr;
    return on;
}

void log(const char* fmt, ...) {
    if (!logOn()) return;
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    using namespace std::chrono;
    static const steady_clock::time_point start = steady_clock::now();
    static std::mutex mu;
    static FILE* file;
    static bool opened;
    std::lock_guard<std::mutex> lock(mu);
    if (!opened) {
        opened = true;
        std::string path = port::files::dataDir() + "/audio.log";
        file = fopen(path.c_str(), "w");
        fprintf(stderr, "medecomp: журнал звука: %s\n", path.c_str());
    }
    fprintf(stderr, "medecomp: %s\n", line);
    if (file) {
        fprintf(file, "%9.3f %s\n", duration<double>(steady_clock::now() - start).count(), line);
        fflush(file);
    }
}

}
}
