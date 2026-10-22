#include "file_dialog.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "port/files.h"
#include "port/platform.h"

#ifdef _WIN32
#include <windows.h>

#include <SDL_syswm.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <shobjidl.h>
#endif

namespace port {
namespace platform {
namespace dialog {

namespace {

std::mutex s_mu;
std::string s_picked;
Kind s_kind = Ipa;
std::atomic<bool> s_busy(false), s_done(false);
bool s_noDialog = false;

std::string shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) out += c == '\'' ? std::string("'\\''") : std::string(1, c);
    return out + "'";
}

bool haveTool(const char* tool) { return system((std::string("command -v ") + tool + " >/dev/null 2>&1").c_str()) == 0; }

struct Filter {
    const char* pattern;
    const char* label;
};

Filter filterOf(Kind kind) {
    switch (kind) {
    case Png: return {"*.png", "PNG"};
    case Gltf: return {"*.glb *.gltf *.vrm", "glTF 2.0, VRM"};
    case Ipa: return {"*.ipa", "Mirror's Edge (.ipa)"};
    case LevelFile: return {"*.glevel", "Glassline (.glevel)"};
    default: return {"", ""};
    }
}

#ifdef _WIN32
std::wstring wide(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(n > 0 ? size_t(n) : 1, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], n);
    out.resize(wcslen(out.c_str()));
    return out;
}

std::string utf8(const wchar_t* s) {
    int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? size_t(n) : 1, '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], n, nullptr, nullptr);
    out.resize(strlen(out.c_str()));
    return out;
}

IShellItem* shellItem(const std::wstring& path) {
    IShellItem* item = nullptr;
    if (path.empty() || FAILED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&item)))) return nullptr;
    return item;
}

bool windowsPick(Kind kind, const std::string& current, const std::string& title, HWND owner, std::string* out) {
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) return false;
    IFileOpenDialog* dialog = nullptr;
    bool made = SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)));
    if (made) {
        DWORD options = 0;
        dialog->GetOptions(&options);
        options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
        options |= kind == MusicFolder ? FOS_PICKFOLDERS : FOS_FILEMUSTEXIST;
        dialog->SetOptions(options);
        dialog->SetTitle(wide(title).c_str());
        Filter filter = filterOf(kind);
        std::wstring pattern = wide(filter.pattern), label = wide(filter.label);
        for (wchar_t& c : pattern)
            if (c == L' ') c = L';';
        if (kind != MusicFolder) {
            COMDLG_FILTERSPEC types[] = {{label.c_str(), pattern.c_str()}, {L"*.*", L"*.*"}};
            dialog->SetFileTypes(2, types);
        }
        std::wstring path = wide(current);
        DWORD attributes = path.empty() ? INVALID_FILE_ATTRIBUTES : GetFileAttributesW(path.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY))
            path = path.substr(0, path.find_last_of(L"/\\"));
        else if (attributes == INVALID_FILE_ATTRIBUTES)
            path.clear();
        if (IShellItem* folder = shellItem(path)) {
            dialog->SetFolder(folder);
            folder->Release();
        } else {
            PWSTR known = nullptr;
            const KNOWNFOLDERID& id = kind == MusicFolder ? FOLDERID_Music : kind == Png ? FOLDERID_Pictures : FOLDERID_Downloads;
            if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &known))) {
                if (IShellItem* folder = shellItem(known)) {
                    dialog->SetDefaultFolder(folder);
                    folder->Release();
                }
                CoTaskMemFree(known);
            }
        }
        if (SUCCEEDED(dialog->Show(owner))) {
            IShellItem* item = nullptr;
            if (SUCCEEDED(dialog->GetResult(&item))) {
                PWSTR chosen = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &chosen))) {
                    *out = utf8(chosen);
                    CoTaskMemFree(chosen);
                }
                item->Release();
            }
        }
        dialog->Release();
    }
    CoUninitialize();
    return made;
}
#endif

}

void pick(Kind kind, const std::string& current, const std::string& title) {
    if (s_busy.exchange(true)) return;
    bool folder = kind == MusicFolder;
    std::string start = current;
    if (!start.empty() && !port::files::stat(start).dir) start = start.substr(0, start.find_last_of('/') + 1);
    if (start.empty() || !port::files::stat(start).exists) {
        const char* home = getenv("HOME");
        start = home ? home : "/";
        const char* sub = kind == MusicFolder ? "/Music" : kind == Png ? "/Pictures" : "/Downloads";
        if (port::files::stat(start + sub).dir) start += sub;
    }
    Filter filter = filterOf(kind);
    void* owner = nullptr;
#ifdef _WIN32
    if (SDL_Window* window = SDL_GetKeyboardFocus()) {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(window, &info)) owner = info.info.win.window;
    }
#endif
    std::thread([start, current, title, folder, kind, filter, owner] {
        std::string result;
        bool ran = false;
#if defined(_WIN32)
        ran = windowsPick(kind, current, title, HWND(owner), &result);
#elif defined(__ANDROID__)
        ran = true;
        const std::string data = port::files::dataDir();
        if (folder) {
            const char* external = SDL_AndroidGetExternalStoragePath();
            result = std::string(external ? external : data.c_str()) + "/music";
            mkdir(result.c_str(), 0755);
        } else {
            result = port::platform::androidPickFile(data + (kind == Ipa ? "/game" : "/imports"),
                                                     kind == Png ? "image/png" : "*/*");
        }
#else
        const char* desktop = getenv("XDG_CURRENT_DESKTOP");
        bool kde = desktop && strstr(desktop, "KDE");
        bool kdialog = haveTool("kdialog"), zenity = haveTool("zenity");
        std::string dir = start + (start.back() == '/' ? "" : "/");
        std::string cmd;
        if (kdialog && (kde || !zenity))
            cmd = folder ? "kdialog --title " + shellQuote(title) + " --getexistingdirectory " + shellQuote(start)
                         : "kdialog --title " + shellQuote(title) + " --getopenfilename " + shellQuote(start) + " " +
                               shellQuote(std::string(filter.pattern) + "|" + filter.label);
        else if (zenity)
            cmd = folder ? "zenity --file-selection --directory --title=" + shellQuote(title) + " --filename=" + shellQuote(dir)
                         : "zenity --file-selection --title=" + shellQuote(title) + " --filename=" + shellQuote(dir) +
                               " --file-filter=" + shellQuote(std::string(filter.label) + " | " + filter.pattern) +
                               " --file-filter=" + shellQuote("* | *");
        if (!cmd.empty())
            if (FILE* p = popen((cmd + " 2>/dev/null").c_str(), "r")) {
                ran = true;
                char buf[4096];
                while (fgets(buf, sizeof buf, p)) result += buf;
                pclose(p);
            }
#endif
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();
        {
            std::lock_guard<std::mutex> lock(s_mu);
            s_picked = result;
            s_kind = kind;
            s_noDialog = !ran;
        }
        s_done = true;
        s_busy = false;
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = SDL_USEREVENT;
        SDL_PushEvent(&e);
    }).detach();
}

bool busy() { return s_busy.load(); }

bool take(Kind* kind, std::string* path, bool* noDialog) {
    if (!s_done.exchange(false)) return false;
    std::lock_guard<std::mutex> lock(s_mu);
    *kind = s_kind;
    *path = s_picked;
    if (noDialog) *noDialog = s_noDialog;
    return true;
}

bool noDialog() {
    std::lock_guard<std::mutex> lock(s_mu);
    return s_noDialog;
}

}
}
}
