#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <chrono>
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
using DeviceCall = int (*)(void*);
using GetIntegerv = void (*)(void*, int, int, int*);

const int kAlcHrtfSoft = 0x1992;

template <class F>
F next(const char* name) {
    return reinterpret_cast<F>(dlsym(RTLD_NEXT, name));
}

std::mutex s_mu;
std::vector<void*> s_devices;
bool s_paused;

void setDevicePaused(void* device, bool paused) {
    GetProc getProc = next<GetProc>("alcGetProcAddress");
    DeviceCall fn = getProc ? reinterpret_cast<DeviceCall>(getProc(device, paused ? "alcDevicePauseSOFT"
                                                                                   : "alcDeviceResumeSOFT"))
                            : nullptr;
    if (fn) fn(device);
}

using BufferData = void (*)(unsigned int, int, const void*, int, int);

void bufferDataStatic(unsigned int buffer, int format, const void* data, int size, int freq) {
    BufferData fn = reinterpret_cast<BufferData>(dlsym(RTLD_NEXT, "alBufferData"));
    if (fn != 0)
        fn(buffer, format, data, size, freq);
}

}

extern "C" void* alGetProcAddress(const char* name) {
    if (name != 0 && strcmp(name, "alBufferDataStatic") == 0)
        return reinterpret_cast<void*>(&bufferDataStatic);
    if (name != 0 && strcmp(name, "alcMacOSXMixerOutputRate") == 0)
        return 0;
    return dlsym(RTLD_NEXT, name);
}

extern "C" void* alcOpenDevice(const char* name) {
    void* device = next<OpenDevice>("alcOpenDevice")(name);
    if (device) {
        std::lock_guard<std::mutex> lock(s_mu);
        s_devices.push_back(device);
        if (s_paused) setDevicePaused(device, true);
    }
    return device;
}

extern "C" int alcCloseDevice(void* device) {
    {
        std::lock_guard<std::mutex> lock(s_mu);
        s_devices.erase(std::remove(s_devices.begin(), s_devices.end(), device), s_devices.end());
    }
    return next<CloseDevice>("alcCloseDevice")(device);
}

extern "C" void* alcCreateContext(void* device, const int* attrs) {
    std::string hrtf = port::platform::settings::get("hrtf", "auto");
    std::vector<int> list;
    for (int i = 0; attrs && attrs[i] != 0; i += 2) {
        if (attrs[i] == kAlcHrtfSoft) continue;
        list.push_back(attrs[i]);
        list.push_back(attrs[i + 1]);
    }
    if (hrtf == "on" || hrtf == "off") {
        list.push_back(kAlcHrtfSoft);
        list.push_back(hrtf == "on" ? 1 : 0);
    }
    list.push_back(0);
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
        int status = -1;
        if (GetIntegerv get = next<GetIntegerv>("alcGetIntegerv")) get(device, kAlcHrtfSoft, 1, &status);
        port::audio::log("звук: контекст OpenAL, объёмный звук (HRTF) %s — настройка «%s», общая громкость %d %%",
                         status == 1 ? "включён" : "выключен", hrtf.c_str(), int(volume * 100 + 0.5f));
    }
    return context;
}

namespace port {
namespace audio {

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
