#include "port/online.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mutex>

#include "../gfx/png.h"
#include "../platform/settings.h"
#include "json.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "../../third_party/stb/stb_image.h"

namespace port {
namespace online {

namespace settings = port::platform::settings;

namespace {

const char* kDefaultServer = "https://game.mooncellar.space";

std::string bearer(const std::string& token) { return "Authorization: Bearer " + token; }

void readAccount(const Json& json, Account* out) {
    out->id = json["id"].str();
    out->userName = json["userName"].str();
    out->avatar = json["avatar"].str();
}

bool openLib(const char* const* names, void** lib) {
    for (const char* const* name = names; *name; name++)
        if ((*lib = dlopen(*name, RTLD_NOW | RTLD_LOCAL))) return true;
    return false;
}

}

std::string serverUrl() {
    const char* env = getenv("MEDECOMP_ONLINE_SERVER");
    const std::string fallback = env && *env ? env : kDefaultServer;
    std::string url = settings::get("online_server", fallback);
    while (!url.empty() && url.back() == '/') url.pop_back();
    return url.empty() ? fallback : url;
}

std::string token() { return settings::get("online_token", ""); }

std::string userName() { return settings::get("online_user", ""); }

void saveAccount(const std::string& token, const std::string& userName) {
    settings::set("online_token", token);
    settings::set("online_user", token.empty() ? std::string() : userName);
    settings::save();
}

std::string deviceName() {
    char host[64] = {};
#if defined(_WIN32)
    medecomp_gethostname(host, sizeof host - 1);
#else
    gethostname(host, sizeof host - 1);
#endif
#if defined(_WIN32)
    std::string os = "Windows";
#elif defined(__APPLE__)
    std::string os = "macOS";
#else
    std::string os = "Linux";
#endif
    std::string name = "PC (" + os + (host[0] ? std::string(", ") + host : std::string()) + ")";
    return name.size() > 64 ? name.substr(0, 64) : name;
}

bool startSignIn(const std::string& server, const std::string& device, DeviceCode* out, std::string* error) {
    Response r = request("POST", server + "/account/device", "{\"deviceName\":" + jsonString(device) + "}");
    Json json;
    if (r.status != 200 || !parseJson(r.body, &json) || json["userCode"].str().empty()) {
        if (error) *error = errorOf(r.status, r.body, r.error);
        return false;
    }
    out->deviceCode = json["deviceCode"].str();
    out->userCode = json["userCode"].str();
    out->url = json["verificationUrl"].str();
    out->urlWithCode = json["verificationUrlComplete"].str(out->url);
    out->expiresIn = int(json["expiresIn"].integer(600));
    out->interval = int(json["interval"].integer(5));
    if (out->interval < 1) out->interval = 5;
    return true;
}

SignIn pollSignIn(const std::string& server, const std::string& deviceCode, std::string* token, Account* account,
                  std::string* error) {
    Response r =
        request("POST", server + "/account/device/token", "{\"deviceCode\":" + jsonString(deviceCode) + "}");
    Json json;
    if (r.status == 200 && parseJson(r.body, &json)) {
        if (json["status"].str() == "pending") return SignIn::Pending;
        if (json["status"].str() == "approved" && !json["token"].str().empty()) {
            *token = json["token"].str();
            readAccount(json["account"], account);
            return SignIn::Approved;
        }
    }
    if (r.status == 0 || r.status >= 500) return SignIn::Pending;
    if (error) *error = errorOf(r.status, r.body, r.error);
    return SignIn::Failed;
}

bool fetchAccount(const std::string& server, const std::string& token, Account* out, bool* revoked,
                  std::string* error) {
    if (revoked) *revoked = false;
    Response r = request("GET", server + "/account/me", "", {bearer(token)});
    Json json;
    if (r.status == 200 && parseJson(r.body, &json)) {
        readAccount(json, out);
        return true;
    }
    if (revoked) *revoked = r.status == 401;
    if (error) *error = errorOf(r.status, r.body, r.error);
    return false;
}

void signOut(const std::string& server, const std::string& token) {
    request("POST", server + "/account/sign-out", "", {bearer(token)}, 5000);
}

namespace {

void printQr(const std::string& text) {
    int width = 0;
    std::vector<bool> modules;
    if (!qrCode(text, &width, &modules)) return;
    auto dark = [&](int x, int y) {
        return x >= 0 && y >= 0 && x < width && y < width && modules[size_t(y) * width + x];
    };
    for (int y = -2; y < width + 2; y += 2) {
        printf("  \033[30;47m");
        for (int x = -2; x < width + 2; x++) {
            bool top = dark(x, y), bottom = dark(x, y + 1);
            printf("%s", top && bottom ? "█" : top ? "▀" : bottom ? "▄" : " ");
        }
        printf("\033[0m\n");
    }
}

void sleepSeconds(int seconds) {
    struct timespec t = {seconds, 0};
    nanosleep(&t, nullptr);
}

}

int consoleSignIn() {
    std::string server = serverUrl(), error;
    DeviceCode code;
    if (!startSignIn(server, deviceName(), &code, &error)) {
        fprintf(stderr, "medecomp: вход: %s\n", error.c_str());
        return 1;
    }
    printf("Вход в MoonCellar. Откройте %s и введите код:\n\n    %s\n\n", code.url.c_str(), code.userCode.c_str());
    printQr(code.urlWithCode);
    printf("\nИли наведите на QR-код камеру телефона. Ждём подтверждения (Ctrl+C — отмена)…\n");
    fflush(stdout);
    time_t deadline = time(nullptr) + code.expiresIn;
    while (time(nullptr) < deadline) {
        sleepSeconds(code.interval);
        std::string token;
        Account account;
        SignIn result = pollSignIn(server, code.deviceCode, &token, &account, &error);
        if (result == SignIn::Approved) {
            saveAccount(token, account.userName);
            printf("Вход выполнен: %s\n", account.userName.c_str());
            return 0;
        }
        if (result == SignIn::Failed) {
            fprintf(stderr, "medecomp: вход: %s\n", error.c_str());
            return 1;
        }
    }
    fprintf(stderr, "medecomp: вход: код истёк\n");
    return 1;
}

int consoleSignOut() {
    std::string current = token();
    if (current.empty()) {
        printf("Вход не выполнен\n");
        return 0;
    }
    signOut(serverUrl(), current);
    saveAccount("", "");
    printf("Выход выполнен\n");
    return 0;
}

bool qrCode(const std::string& text, int* width, std::vector<bool>* modules) {
    struct QRcode {
        int version;
        int width;
        unsigned char* data;
    };
    static std::mutex mu;
    static bool tried;
    static QRcode* (*encode)(const char*, int, int, int, int);
    static void (*release)(QRcode*);
    {
        std::lock_guard<std::mutex> lock(mu);
        if (!tried) {
            tried = true;
            static const char* const names[] = {"libqrencode.so.4", "libqrencode.so", nullptr};
            void* lib = nullptr;
            if (openLib(names, &lib)) {
                encode = (QRcode * (*)(const char*, int, int, int, int)) dlsym(lib, "QRcode_encodeString");
                release = (void (*)(QRcode*))dlsym(lib, "QRcode_free");
            }
        }
    }
    if (!encode || !release) return false;
    const int kLevelM = 1, kMode8 = 2;
    QRcode* qr = encode(text.c_str(), 0, kLevelM, kMode8, 1);
    if (!qr) return false;
    *width = qr->width;
    modules->assign(size_t(qr->width) * qr->width, false);
    for (size_t i = 0; i < modules->size(); i++) (*modules)[i] = qr->data[i] & 1;
    release(qr);
    return true;
}

bool decodeImage(const std::string& bytes, int* width, int* height, std::vector<uint8_t>* rgba) {
    port::gfx::Image png;
    if (port::gfx::decodePNG(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), png)) {
        *width = png.width;
        *height = png.height;
        *rgba = std::move(png.rgba);
        return true;
    }
    int w = 0, h = 0, channels = 0;
    if (stbi_uc* px = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()), int(bytes.size()), &w, &h,
                                            &channels, 4)) {
        *width = w;
        *height = h;
        rgba->assign(px, px + size_t(w) * size_t(h) * 4);
        stbi_image_free(px);
        return true;
    }
    if (bytes.size() < 12 || bytes.compare(0, 4, "RIFF") != 0 || bytes.compare(8, 4, "WEBP") != 0) return false;
    static std::mutex mu;
    static bool tried;
    static struct {
        void* (*loaderNew)(const char*, void**);
        int (*write)(void*, const unsigned char*, size_t, void**);
        int (*close)(void*, void**);
        void* (*pixbuf)(void*);
        int (*width)(const void*);
        int (*height)(const void*);
        int (*stride)(const void*);
        int (*channels)(const void*);
        unsigned char* (*pixels)(const void*);
        void (*unref)(void*);
    } gdk;
    static bool ok;
    {
        std::lock_guard<std::mutex> lock(mu);
        if (!tried) {
            tried = true;
            static const char* const names[] = {"libgdk_pixbuf-2.0.so.0", nullptr};
            void* lib = nullptr;
            if (openLib(names, &lib)) {
                struct {
                    const char* name;
                    void** slot;
                } syms[] = {
                    {"gdk_pixbuf_loader_new_with_type", (void**)&gdk.loaderNew},
                    {"gdk_pixbuf_loader_write", (void**)&gdk.write},
                    {"gdk_pixbuf_loader_close", (void**)&gdk.close},
                    {"gdk_pixbuf_loader_get_pixbuf", (void**)&gdk.pixbuf},
                    {"gdk_pixbuf_get_width", (void**)&gdk.width},
                    {"gdk_pixbuf_get_height", (void**)&gdk.height},
                    {"gdk_pixbuf_get_rowstride", (void**)&gdk.stride},
                    {"gdk_pixbuf_get_n_channels", (void**)&gdk.channels},
                    {"gdk_pixbuf_get_pixels", (void**)&gdk.pixels},
                    {"g_object_unref", (void**)&gdk.unref},
                };
                ok = true;
                for (auto& s : syms) ok = ok && (*s.slot = dlsym(lib, s.name)) != nullptr;
            }
        }
    }
    if (!ok || bytes.empty()) return false;
    void* loader = gdk.loaderNew("webp", nullptr);
    if (!loader) return false;
    bool done = gdk.write(loader, reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), nullptr);
    done = gdk.close(loader, nullptr) && done;
    void* pixbuf = done ? gdk.pixbuf(loader) : nullptr;
    if (pixbuf) {
        int w = gdk.width(pixbuf), h = gdk.height(pixbuf), stride = gdk.stride(pixbuf), n = gdk.channels(pixbuf);
        const unsigned char* src = gdk.pixels(pixbuf);
        if (w > 0 && h > 0 && (n == 3 || n == 4) && src) {
            *width = w;
            *height = h;
            rgba->resize(size_t(w) * h * 4);
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++) {
                    const unsigned char* p = src + size_t(y) * stride + size_t(x) * n;
                    uint8_t* q = &(*rgba)[(size_t(y) * w + x) * 4];
                    q[0] = p[0];
                    q[1] = p[1];
                    q[2] = p[2];
                    q[3] = n == 4 ? p[3] : 255;
                }
        } else {
            pixbuf = nullptr;
        }
    }
    gdk.unref(loader);
    return pixbuf != nullptr;
}

}
}
