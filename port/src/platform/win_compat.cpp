#ifdef _WIN32

#include <windows.h>
#include <psapi.h>

#include <string.h>

#include <string>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>

#include "port/files.h"
#include "port/platform.h"

namespace {

std::wstring wide(const char* utf8) {
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    std::wstring out(n > 0 ? size_t(n - 1) : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &out[0], n);
    return out;
}

std::string narrow(const wchar_t* text) {
    int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? size_t(n - 1) : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, text, -1, &out[0], n, nullptr, nullptr);
    return out;
}

thread_local std::string s_error;

void setError(const std::string& what) {
    char text[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), 0, text,
                   sizeof text, nullptr);
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) text[--len] = 0;
    s_error = what + ": " + text;
}

HMODULE load(const std::string& name) {
    std::wstring w = wide(name.c_str());
    for (wchar_t& c : w)
        if (c == L'/') c = L'\\';
    return LoadLibraryW(w.c_str());
}

}

extern "C" void* dlopen(const char* file, int) {
    if (!file) return GetModuleHandleW(nullptr);
    std::string name = file;
    if (HMODULE m = load(name)) return m;
    size_t so = name.find(".so");
    if (so != std::string::npos) {
        std::string base = name.substr(0, so);
        std::string version = so + 3 < name.size() && name[so + 3] == '.' ? name.substr(so + 4) : std::string();
        size_t dot = version.find('.');
        if (dot != std::string::npos) version.resize(dot);
        if (!version.empty())
            if (HMODULE m = load(base + "-" + version + ".dll")) return m;
        if (HMODULE m = load(base + ".dll")) return m;
    }
    setError(name);
    return nullptr;
}

extern "C" void* dlsym(void* handle, const char* name) {
    if (handle != RTLD_NEXT && handle != RTLD_DEFAULT) {
        void* f = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
        if (!f) setError(name);
        return f;
    }
    HMODULE self = GetModuleHandleW(nullptr);
    HMODULE modules[1024];
    DWORD bytes = 0;
    if (K32EnumProcessModules(GetCurrentProcess(), modules, sizeof modules, &bytes)) {
        for (DWORD i = 0; i < bytes / sizeof(HMODULE) && i < 1024; i++) {
            if (modules[i] == self) continue;
            if (void* f = reinterpret_cast<void*>(GetProcAddress(modules[i], name))) return f;
        }
    }
    s_error = std::string(name) + ": нет ни в одной загруженной библиотеке";
    return nullptr;
}

extern "C" int dlclose(void* handle) { return FreeLibrary(static_cast<HMODULE>(handle)) ? 0 : -1; }

extern "C" char* dlerror(void) {
    if (s_error.empty()) return nullptr;
    static thread_local std::string last;
    last.swap(s_error);
    s_error.clear();
    return &last[0];
}

extern "C" int statvfs(const char* path, struct statvfs* out) {
    ULARGE_INTEGER available, total, free;
    if (!GetDiskFreeSpaceExW(wide(path).c_str(), &available, &total, &free)) return -1;
    out->f_bsize = out->f_frsize = 1;
    out->f_blocks = total.QuadPart;
    out->f_bfree = free.QuadPart;
    out->f_bavail = available.QuadPart;
    return 0;
}

extern "C" int medecomp_gethostname(char* name, size_t size) {
    wchar_t host[256];
    DWORD n = sizeof host / sizeof host[0];
    if (!GetComputerNameW(host, &n)) return -1;
    std::string utf8 = narrow(host);
    if (size == 0) return -1;
    size_t len = utf8.size() < size - 1 ? utf8.size() : size - 1;
    memcpy(name, utf8.data(), len);
    name[len] = 0;
    return 0;
}

extern "C" void* valloc(size_t size) { return malloc(size); }

namespace port {
namespace platform {

void windowsStartup() {
    HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
    bool child = getenv("MEDECOMP_LOG_FILE") != nullptr;
    _putenv_s("MEDECOMP_LOG_FILE", "-");
    if (!child && (!err || err == INVALID_HANDLE_VALUE || GetFileType(err) == FILE_TYPE_UNKNOWN)) {
        std::string log = port::files::dataDir() + "/medecomp.log";
        if (freopen(log.c_str(), "w", stderr)) setvbuf(stderr, nullptr, _IONBF, 0);
        if (freopen(log.c_str(), "a", stdout)) setvbuf(stdout, nullptr, _IONBF, 0);
    }
    if (!getenv("HOME")) {
        if (const char* profile = getenv("USERPROFILE")) {
            std::string home = profile;
            for (char& c : home)
                if (c == '\\') c = '/';
            _putenv_s("HOME", home.c_str());
        }
    }
    if (!getenv("FONTCONFIG_FILE") && !getenv("FONTCONFIG_PATH")) {
        char exe[4096];
        ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
        if (n > 0) {
            std::string dir(exe, size_t(n));
            dir.erase(dir.find_last_of('/'));
            _putenv_s("FONTCONFIG_PATH", (dir + "/etc/fonts").c_str());
        }
    }
}

}
}

extern "C" ssize_t readlink(const char* path, char* buf, size_t size) {
    if (strcmp(path, "/proc/self/exe") != 0) return -1;
    wchar_t exe[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, exe, DWORD(sizeof exe / sizeof exe[0]));
    if (n == 0) return -1;
    std::string utf8 = narrow(exe);
    for (char& c : utf8)
        if (c == '\\') c = '/';
    size_t len = utf8.size() < size ? utf8.size() : size;
    memcpy(buf, utf8.data(), len);
    return ssize_t(len);
}

#endif
