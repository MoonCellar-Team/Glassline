#include "port/online.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/em_asm.h>
#include <emscripten/fetch.h>
#else
#include <dlfcn.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <strings.h>
#else
#define strncasecmp _strnicmp
#endif
#include <time.h>

#include <chrono>
#include <mutex>

#include "json.h"

namespace port {
namespace online {

namespace {

#ifndef __EMSCRIPTEN__
const int kOptWriteData = 10001, kOptUrl = 10002, kOptUserAgent = 10018, kOptHttpHeader = 10023,
          kOptCustomRequest = 10036, kOptFollowLocation = 52, kOptNoSignal = 99, kOptAcceptEncoding = 10102,
          kOptTimeoutMs = 155, kOptConnectTimeoutMs = 156, kOptCopyPostFields = 10165, kOptPostFieldSize = 60,
          kOptWriteFunction = 20011;
const int kInfoResponseCode = 0x200000 + 2;
const int kOptConnectOnly = 141;
const int kOptNoProgress = 43, kOptFailOnError = 45, kOptProtocols = 181, kOptRedirProtocols = 182,
          kOptLowSpeedLimit = 19, kOptLowSpeedTime = 20, kOptXferInfoData = 10057, kOptXferInfoFunction = 20219,
          kOptMaxFileSizeLarge = 30117;
const long kProtoHttp = 1 << 0;
const long kProtoHttps = 1 << 1;
const int kOptProtocolsStr = 10318, kOptRedirProtocolsStr = 10319;
const int kCurlHttpError = 22;
const int kCurlAborted = 42;
const long long kDownloadMax = 4LL << 30;
const int kOptSslOptions = 216;
const int kOptCaInfo = 10065;
const long kSslNativeCa = 1 << 4;
const int kCurlAgain = 81;
const unsigned kWsText = 1 << 0, kWsCont = 1 << 2, kWsClose = 1 << 3, kWsPing = 1 << 4, kWsPong = 1 << 6;

struct WsFrame {
    int age;
    int flags;
    long long offset;
    long long bytesleft;
    size_t len;
};
const long kGlobalDefault = 3;

struct Slist;
struct Curl {
    int (*globalInit)(long);
    void* (*init)();
    int (*setopt)(void*, int, ...);
    int (*perform)(void*);
    int (*getinfo)(void*, int, ...);
    void (*cleanup)(void*);
    const char* (*strerror)(int);
    Slist* (*slistAppend)(Slist*, const char*);
    void (*slistFree)(Slist*);
    int (*wsRecv)(void*, void*, size_t, size_t*, const WsFrame**);
    int (*wsSend)(void*, const void*, size_t, size_t*, long long, unsigned);
};

#ifndef _WIN32
const char* caBundle() {
    static const char* found = [] {
        static std::string env;
        if (const char* e = getenv("SSL_CERT_FILE"); e && *e) return (env = e).c_str();
        static const char* const paths[] = {
            "/etc/ssl/certs/ca-certificates.crt",
            "/etc/pki/tls/certs/ca-bundle.crt",
            "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
            "/etc/ssl/ca-bundle.pem",
            "/etc/ssl/cert.pem",
        };
        for (const char* p : paths)
            if (access(p, R_OK) == 0) return p;
        return static_cast<const char*>(nullptr);
    }();
    return found;
}
#endif

std::mutex s_mu;
bool s_tried, s_ok;
std::string s_why;
Curl s_curl;

bool load() {
    std::lock_guard<std::mutex> lock(s_mu);
    if (s_tried) return s_ok;
    s_tried = true;
    const char* names[] = {getenv("MEDECOMP_CURL"), "libcurl.so.4", "libcurl-gnutls.so.4", "libcurl.so"};
    void* lib = nullptr;
    for (const char* name : names)
        if (name && *name && (lib = dlopen(name, RTLD_NOW | RTLD_LOCAL))) break;
    if (!lib) {
        s_why = "нет libcurl (пакет libcurl4 или curl)";
        return false;
    }
    struct {
        const char* name;
        void** slot;
    } syms[] = {
        {"curl_global_init", (void**)&s_curl.globalInit}, {"curl_easy_init", (void**)&s_curl.init},
        {"curl_easy_setopt", (void**)&s_curl.setopt},     {"curl_easy_perform", (void**)&s_curl.perform},
        {"curl_easy_getinfo", (void**)&s_curl.getinfo},   {"curl_easy_cleanup", (void**)&s_curl.cleanup},
        {"curl_easy_strerror", (void**)&s_curl.strerror}, {"curl_slist_append", (void**)&s_curl.slistAppend},
        {"curl_slist_free_all", (void**)&s_curl.slistFree},
    };
    for (auto& s : syms) {
        *s.slot = dlsym(lib, s.name);
        if (!*s.slot) {
            s_why = std::string("в libcurl нет ") + s.name;
            return false;
        }
    }
    s_curl.wsRecv = (int (*)(void*, void*, size_t, size_t*, const WsFrame**))dlsym(lib, "curl_ws_recv");
    s_curl.wsSend = (int (*)(void*, const void*, size_t, size_t*, long long, unsigned))dlsym(lib, "curl_ws_send");
    if (s_curl.globalInit(kGlobalDefault) != 0) {
        s_why = "curl_global_init не удался";
        return false;
    }
    s_ok = true;
    return true;
}

size_t collect(char* data, size_t size, size_t count, void* user) {
    static_cast<std::string*>(user)->append(data, size * count);
    return size * count;
}

#endif

struct Parser {
    const std::string& s;
    size_t i = 0;

    explicit Parser(const std::string& text) : s(text) {}

    void space() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) i++;
    }

    bool literal(const char* word) {
        size_t n = strlen(word);
        if (s.compare(i, n, word) != 0) return false;
        i += n;
        return true;
    }

    void utf8(unsigned code, std::string* out) {
        if (code < 0x80) {
            *out += char(code);
        } else if (code < 0x800) {
            *out += char(0xc0 | (code >> 6));
            *out += char(0x80 | (code & 0x3f));
        } else if (code < 0x10000) {
            *out += char(0xe0 | (code >> 12));
            *out += char(0x80 | ((code >> 6) & 0x3f));
            *out += char(0x80 | (code & 0x3f));
        } else {
            *out += char(0xf0 | (code >> 18));
            *out += char(0x80 | ((code >> 12) & 0x3f));
            *out += char(0x80 | ((code >> 6) & 0x3f));
            *out += char(0x80 | (code & 0x3f));
        }
    }

    bool hex4(unsigned* out) {
        if (i + 4 > s.size()) return false;
        *out = unsigned(strtoul(s.substr(i, 4).c_str(), nullptr, 16));
        i += 4;
        return true;
    }

    bool string(std::string* out) {
        if (i >= s.size() || s[i] != '"') return false;
        i++;
        while (i < s.size() && s[i] != '"') {
            char c = s[i++];
            if (c != '\\') {
                *out += c;
                continue;
            }
            if (i >= s.size()) return false;
            char e = s[i++];
            switch (e) {
            case 'n': *out += '\n'; break;
            case 't': *out += '\t'; break;
            case 'r': *out += '\r'; break;
            case 'b': *out += '\b'; break;
            case 'f': *out += '\f'; break;
            case 'u': {
                unsigned code;
                if (!hex4(&code)) return false;
                if (code >= 0xd800 && code < 0xdc00 && s.compare(i, 2, "\\u") == 0) {
                    i += 2;
                    unsigned low;
                    if (!hex4(&low)) return false;
                    code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);
                }
                utf8(code, out);
                break;
            }
            default: *out += e;
            }
        }
        if (i >= s.size()) return false;
        i++;
        return true;
    }

    bool value(Json* out, int depth) {
        if (depth > 64) return false;
        space();
        if (i >= s.size()) return false;
        char c = s[i];
        if (c == '{') {
            out->type = Json::Object;
            i++;
            space();
            if (i < s.size() && s[i] == '}') return ++i, true;
            for (;;) {
                space();
                std::pair<std::string, Json> field;
                if (!string(&field.first)) return false;
                space();
                if (i >= s.size() || s[i++] != ':') return false;
                if (!value(&field.second, depth + 1)) return false;
                out->fields.push_back(std::move(field));
                space();
                if (i < s.size() && s[i] == ',') {
                    i++;
                    continue;
                }
                return i < s.size() && s[i++] == '}';
            }
        }
        if (c == '[') {
            out->type = Json::Array;
            i++;
            space();
            if (i < s.size() && s[i] == ']') return ++i, true;
            for (;;) {
                Json item;
                if (!value(&item, depth + 1)) return false;
                out->items.push_back(std::move(item));
                space();
                if (i < s.size() && s[i] == ',') {
                    i++;
                    continue;
                }
                return i < s.size() && s[i++] == ']';
            }
        }
        if (c == '"') {
            out->type = Json::String;
            return string(&out->string);
        }
        if (literal("true")) return out->type = Json::Bool, out->boolean = true, true;
        if (literal("false")) return out->type = Json::Bool, true;
        if (literal("null")) return true;
        char* end = nullptr;
        out->number = strtod(s.c_str() + i, &end);
        if (end == s.c_str() + i) return false;
        out->type = Json::Number;
        i = size_t(end - s.c_str());
        return true;
    }
};

const char kBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

}

const Json& Json::operator[](const char* key) const {
    static const Json none;
    for (const auto& f : fields)
        if (f.first == key) return f.second;
    return none;
}

const Json& Json::operator[](size_t index) const {
    static const Json none;
    return index < items.size() ? items[index] : none;
}

std::string Json::str(const std::string& fallback) const { return type == String ? string : fallback; }

long long Json::integer(long long fallback) const { return type == Number ? (long long)number : fallback; }

bool parseJson(const std::string& text, Json* out) {
    Parser p(text);
    *out = Json();
    if (!p.value(out, 0)) return false;
    p.space();
    return p.i == text.size();
}

std::string jsonString(const std::string& text) {
    std::string out = "\"";
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += char(c);
        } else if (c < 0x20) {
            char buf[8];
            snprintf(buf, sizeof buf, "\\u%04x", c);
            out += buf;
        } else {
            out += char(c);
        }
    }
    return out + "\"";
}

std::string base64Encode(const void* data, size_t size) {
    const unsigned char* p = static_cast<const unsigned char*>(data);
    std::string out;
    out.reserve((size + 2) / 3 * 4);
    for (size_t i = 0; i < size; i += 3) {
        unsigned v = unsigned(p[i]) << 16;
        if (i + 1 < size) v |= unsigned(p[i + 1]) << 8;
        if (i + 2 < size) v |= p[i + 2];
        out += kBase64[(v >> 18) & 63];
        out += kBase64[(v >> 12) & 63];
        out += i + 1 < size ? kBase64[(v >> 6) & 63] : '=';
        out += i + 2 < size ? kBase64[v & 63] : '=';
    }
    return out;
}

bool base64Decode(const std::string& text, std::string* out) {
    out->clear();
    unsigned v = 0;
    int bits = 0;
    for (char c : text) {
        if (c == '=') break;
        const char* at = strchr(kBase64, c);
        if (!at || !c) return false;
        v = (v << 6) | unsigned(at - kBase64);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            *out += char((v >> bits) & 0xff);
        }
    }
    return true;
}

std::string errorOf(int status, const std::string& body, const std::string& transportError) {
    if (status == 0) return transportError.empty() ? "нет связи с сервером" : transportError;
    Json json;
    if (parseJson(body, &json) && !json["message"].str().empty()) return json["message"].str();
    return "сервер ответил " + std::to_string(status);
}

#ifndef __EMSCRIPTEN__

bool available(std::string* why) {
    bool ok = load();
    if (!ok && why) *why = s_why;
    return ok;
}

namespace {

bool loopback(const std::string& url) {
    size_t at = url.find("://");
    if (at == std::string::npos) return false;
    std::string host = url.substr(at + 3);
    for (const char* name : {"localhost", "127.0.0.1", "[::1]"}) {
        size_t n = strlen(name);
        if (strncasecmp(host.c_str(), name, n) == 0 && (host.size() == n || host[n] == ':' || host[n] == '/'))
            return true;
    }
    return false;
}

void onlySecure(void* h, const std::string& url, bool ws) {
    const bool local = loopback(url);
    const char* allowed = ws ? (local ? "ws,wss" : "wss") : (local ? "http,https" : "https");
    if (s_curl.setopt(h, kOptProtocolsStr, allowed) != 0 && !ws)
        s_curl.setopt(h, kOptProtocols, local ? kProtoHttp | kProtoHttps : kProtoHttps);
    if (s_curl.setopt(h, kOptRedirProtocolsStr, ws ? "wss" : "https") != 0 && !ws)
        s_curl.setopt(h, kOptRedirProtocols, kProtoHttps);
}

}

Response request(const std::string& method, const std::string& url, const std::string& body,
                 const std::vector<std::string>& headers, long timeoutMs) {
    Response r;
    if (!load()) {
        r.error = s_why;
        return r;
    }
    void* h = s_curl.init();
    if (!h) {
        r.error = "curl_easy_init не удался";
        return r;
    }
    Slist* list = nullptr;
    bool typed = false;
    for (const std::string& header : headers) {
        list = s_curl.slistAppend(list, header.c_str());
        typed |= strncasecmp(header.c_str(), "Content-Type:", 13) == 0;
    }
    if (!body.empty() && !typed) list = s_curl.slistAppend(list, "Content-Type: application/json");
    s_curl.setopt(h, kOptUrl, url.c_str());
    onlySecure(h, url, false);
    s_curl.setopt(h, kOptCustomRequest, method.c_str());
    if (!body.empty() || method == "POST" || method == "PUT") {
        s_curl.setopt(h, kOptPostFieldSize, long(body.size()));
        s_curl.setopt(h, kOptCopyPostFields, body.c_str());
    }
    if (list) s_curl.setopt(h, kOptHttpHeader, list);
    s_curl.setopt(h, kOptUserAgent, "medecomp-port");
    s_curl.setopt(h, kOptAcceptEncoding, "");
    s_curl.setopt(h, kOptFollowLocation, 1L);
    s_curl.setopt(h, kOptNoSignal, 1L);
#ifdef _WIN32
    s_curl.setopt(h, kOptSslOptions, kSslNativeCa);
#else
    if (const char* ca = caBundle()) s_curl.setopt(h, kOptCaInfo, ca);
#endif
    s_curl.setopt(h, kOptTimeoutMs, timeoutMs);
    s_curl.setopt(h, kOptConnectTimeoutMs, timeoutMs < 10000 ? timeoutMs : 10000L);
    s_curl.setopt(h, kOptWriteFunction, &collect);
    s_curl.setopt(h, kOptWriteData, &r.body);
    int code = s_curl.perform(h);
    if (code == 0) {
        long status = 0;
        s_curl.getinfo(h, kInfoResponseCode, &status);
        r.status = int(status);
    } else {
        r.error = s_curl.strerror(code);
        r.body.clear();
    }
    s_curl.cleanup(h);
    if (list) s_curl.slistFree(list);
    return r;
}

namespace {

struct DownloadState {
    FILE* file;
    const std::function<bool(long long, long long)>* progress;
};

size_t writeFile(char* data, size_t size, size_t count, void* user) {
    return fwrite(data, size, count, static_cast<DownloadState*>(user)->file) * size;
}

int xferInfo(void* user, long long total, long long now, long long, long long) {
    const auto* progress = static_cast<DownloadState*>(user)->progress;
    return *progress && !(*progress)(now, total) ? 1 : 0;
}

}

bool download(const std::string& url, const std::string& path,
              const std::function<bool(long long done, long long total)>& progress, std::string* error, bool localHttp) {
    const bool local = localHttp && (strncasecmp(url.c_str(), "http://127.0.0.1:", 17) == 0 ||
                                     strncasecmp(url.c_str(), "http://localhost:", 17) == 0);
    if (!local && strncasecmp(url.c_str(), "https://", 8) != 0) return *error = "нужен адрес https://", false;
    if (!load()) return *error = s_why, false;
    FILE* file = fopen(path.c_str(), "wb");
    if (!file) return *error = "не создать " + path, false;
    void* h = s_curl.init();
    if (!h) {
        fclose(file);
        return *error = "curl_easy_init не удался", false;
    }
    DownloadState state{file, &progress};
    s_curl.setopt(h, kOptUrl, url.c_str());
    s_curl.setopt(h, kOptProtocols, local ? kProtoHttp | kProtoHttps : kProtoHttps);
    s_curl.setopt(h, kOptRedirProtocols, local ? kProtoHttp | kProtoHttps : kProtoHttps);
    s_curl.setopt(h, kOptFollowLocation, 1L);
    s_curl.setopt(h, kOptFailOnError, 1L);
    s_curl.setopt(h, kOptNoSignal, 1L);
    s_curl.setopt(h, kOptUserAgent, "medecomp-port");
#ifdef _WIN32
    s_curl.setopt(h, kOptSslOptions, kSslNativeCa);
#else
    if (const char* ca = caBundle()) s_curl.setopt(h, kOptCaInfo, ca);
#endif
    s_curl.setopt(h, kOptConnectTimeoutMs, 15000L);
    s_curl.setopt(h, kOptLowSpeedLimit, 1L);
    s_curl.setopt(h, kOptLowSpeedTime, 60L);
    s_curl.setopt(h, kOptMaxFileSizeLarge, kDownloadMax);
    s_curl.setopt(h, kOptWriteFunction, &writeFile);
    s_curl.setopt(h, kOptWriteData, &state);
    s_curl.setopt(h, kOptNoProgress, 0L);
    s_curl.setopt(h, kOptXferInfoFunction, &xferInfo);
    s_curl.setopt(h, kOptXferInfoData, &state);
    int code = s_curl.perform(h);
    long status = 0;
    s_curl.getinfo(h, kInfoResponseCode, &status);
    s_curl.cleanup(h);
    bool written = fclose(file) == 0;
    if (code == kCurlAborted) return *error = "отменено", false;
    if (code == kCurlHttpError) return *error = "сервер ответил " + std::to_string(status), false;
    if (code != 0) return *error = s_curl.strerror(code), false;
    if (!written) return *error = "не записать " + path, false;
    return true;
}

bool WebSocket::open(const std::string& url, const std::vector<std::string>& headers, std::string* error) {
    close();
    if (!load()) {
        if (error) *error = s_why;
        return false;
    }
    if (!s_curl.wsRecv || !s_curl.wsSend) {
        if (error) *error = "libcurl без WebSocket (нужна 7.86 и новее)";
        return false;
    }
    void* h = s_curl.init();
    if (!h) {
        if (error) *error = "curl_easy_init не удался";
        return false;
    }
    Slist* list = nullptr;
    for (const std::string& header : headers) list = s_curl.slistAppend(list, header.c_str());
    s_curl.setopt(h, kOptUrl, url.c_str());
    onlySecure(h, url, true);
    if (list) s_curl.setopt(h, kOptHttpHeader, list);
    s_curl.setopt(h, kOptUserAgent, "medecomp-port");
    s_curl.setopt(h, kOptConnectOnly, 2L);
    s_curl.setopt(h, kOptNoSignal, 1L);
#ifdef _WIN32
    s_curl.setopt(h, kOptSslOptions, kSslNativeCa);
#else
    if (const char* ca = caBundle()) s_curl.setopt(h, kOptCaInfo, ca);
#endif
    s_curl.setopt(h, kOptConnectTimeoutMs, 10000L);
    int code = s_curl.perform(h);
    if (code != 0) {
        if (error) *error = s_curl.strerror(code);
        s_curl.cleanup(h);
        if (list) s_curl.slistFree(list);
        return false;
    }
    m_handle = h;
    m_headers = list;
    m_partial.clear();
    m_pingAt = 0;
    m_rtt = -1;
    return true;
}

namespace {
long long steadyMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
}

bool WebSocket::ping() {
    if (!m_handle) return false;
    long long now = steadyMs();
    if (m_pingAt && now - m_pingAt < 5000) return false;
    size_t sent = 0;
    if (s_curl.wsSend(m_handle, "mc", 2, &sent, 0, kWsPing) != 0) return false;
    m_pingAt = now;
    return true;
}

bool WebSocket::send(const std::string& text) {
    if (!m_handle) return false;
    for (int attempt = 0; attempt < 500; attempt++) {
        size_t sent = 0;
        int code = s_curl.wsSend(m_handle, text.data(), text.size(), &sent, 0, kWsText);
        if (code == 0) return true;
        if (code != kCurlAgain) return false;
        struct timespec t = {0, 2000000};
        nanosleep(&t, nullptr);
    }
    return false;
}

bool WebSocket::recv(std::string* text, bool* closed) {
    *closed = false;
    if (!m_handle) {
        *closed = true;
        return false;
    }
    char buf[4096];
    for (;;) {
        size_t got = 0;
        const WsFrame* meta = nullptr;
        int code = s_curl.wsRecv(m_handle, buf, sizeof buf, &got, &meta);
        if (code == kCurlAgain) return false;
        if (code != 0 || !meta || (meta->flags & kWsClose)) {
            *closed = true;
            return false;
        }
        if (meta->flags & kWsPong) {
            if (m_pingAt) m_rtt = int(steadyMs() - m_pingAt);
            m_pingAt = 0;
            continue;
        }
        if (meta->flags & kWsPing) continue;
        m_partial.append(buf, got);
        if (meta->bytesleft == 0 && !(meta->flags & kWsCont)) {
            *text = std::move(m_partial);
            m_partial.clear();
            return true;
        }
    }
}

void WebSocket::close() {
    if (m_handle) {
        size_t sent = 0;
        s_curl.wsSend(m_handle, "", 0, &sent, 0, kWsClose);
        s_curl.cleanup(m_handle);
    }
    if (m_headers) s_curl.slistFree(static_cast<Slist*>(m_headers));
    m_handle = nullptr;
    m_headers = nullptr;
    m_partial.clear();
    m_pingAt = 0;
    m_rtt = -1;
}

#else

bool available(std::string*) { return true; }

namespace {

struct FetchHeaders {
    std::vector<std::string> parts;
    std::vector<const char*> ptrs;

    FetchHeaders(const std::vector<std::string>& headers, bool json) {
        bool typed = false;
        for (const std::string& header : headers) {
            size_t colon = header.find(':');
            if (colon == std::string::npos) continue;
            std::string name = header.substr(0, colon), value = header.substr(colon + 1);
            while (!value.empty() && value[0] == ' ') value.erase(0, 1);
            typed |= strncasecmp(name.c_str(), "Content-Type", 12) == 0;
            parts.push_back(name);
            parts.push_back(value);
        }
        if (json && !typed) {
            parts.push_back("Content-Type");
            parts.push_back("application/json");
        }
        for (const std::string& part : parts) ptrs.push_back(part.c_str());
        ptrs.push_back(nullptr);
    }
};

Response fetchSync(const std::string& method, const std::string& url, const std::string& body,
                   const std::vector<std::string>& headers, long timeoutMs) {
    Response r;
    FetchHeaders list(headers, !body.empty());
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    snprintf(attr.requestMethod, sizeof attr.requestMethod, "%s", method.c_str());
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_REPLACE;
    attr.requestHeaders = list.ptrs.data();
    if (!body.empty() || method == "POST" || method == "PUT") {
        attr.requestData = body.data();
        attr.requestDataSize = body.size();
    }
    attr.timeoutMSecs = unsigned(timeoutMs > 0 ? timeoutMs : 0);
    emscripten_fetch_t* f = emscripten_fetch(&attr, url.c_str());
    if (!f) {
        r.error = "запрос не отправить";
        return r;
    }
    r.status = f->status;
    if (r.status != 0 && f->data) r.body.assign(f->data, size_t(f->numBytes));
    if (r.status == 0) r.error = f->statusText[0] ? f->statusText : "нет связи с сервером";
    emscripten_fetch_close(f);
    return r;
}

}

Response request(const std::string& method, const std::string& url, const std::string& body,
                 const std::vector<std::string>& headers, long timeoutMs) {
    return fetchSync(method, url, body, headers, timeoutMs);
}

bool download(const std::string& url, const std::string& path,
              const std::function<bool(long long done, long long total)>& progress, std::string* error, bool) {
    Response r = fetchSync("GET", url, std::string(), {}, 0);
    if (r.status == 0) return *error = r.error, false;
    if (r.status >= 400) return *error = "сервер ответил " + std::to_string(r.status), false;
    if (progress && !progress((long long)r.body.size(), (long long)r.body.size())) return *error = "отменено", false;
    FILE* file = fopen(path.c_str(), "wb");
    if (!file) return *error = "не создать " + path, false;
    bool written = fwrite(r.body.data(), 1, r.body.size(), file) == r.body.size();
    written = fclose(file) == 0 && written;
    if (!written) return *error = "не записать " + path, false;
    return true;
}

bool WebSocket::open(const std::string& url, const std::vector<std::string>& headers, std::string* error) {
    close();
    std::string target = url;
    for (const std::string& header : headers)
        if (strncasecmp(header.c_str(), "Authorization: Bearer ", 22) == 0)
            target += std::string(target.find('?') == std::string::npos ? "?" : "&") + "token=" + header.substr(22);
    int id = MAIN_THREAD_EM_ASM_INT({
        const all = Module.medecompSockets || (Module.medecompSockets = { next: 1 });
        const id = all.next++;
        const s = {}; s.queue = []; s.state = 0;
        all[id] = s;
        try {
            s.ws = new WebSocket(UTF8ToString($0));
        } catch (e) {
            s.state = 2;
            return id;
        }
        s.ws.onopen = () => { s.state = 1; };
        s.ws.onmessage = (e) => { if (typeof e.data === "string") s.queue.push(e.data); };
        s.ws.onclose = () => { s.state = 2; };
        s.ws.onerror = () => { s.state = 2; };
        return id;
    }, target.c_str());
    int state = 0;
    for (int waited = 0; waited < 10000 && state == 0; waited += 10) {
        state = MAIN_THREAD_EM_ASM_INT({ return Module.medecompSockets[$0].state; }, id);
        if (state == 0) {
            struct timespec t = {0, 10000000};
            nanosleep(&t, nullptr);
        }
    }
    if (state != 1) {
        MAIN_THREAD_EM_ASM({
            const s = Module.medecompSockets[$0];
            if (s.ws) s.ws.close();
            delete Module.medecompSockets[$0];
        }, id);
        if (error) *error = state == 0 ? "сервер не ответил" : "соединение не установлено";
        return false;
    }
    m_handle = reinterpret_cast<void*>(intptr_t(id));
    m_partial.clear();
    m_pingAt = 0;
    m_rtt = -1;
    return true;
}

bool WebSocket::ping() { return false; }

bool WebSocket::send(const std::string& text) {
    if (!m_handle) return false;
    return MAIN_THREAD_EM_ASM_INT({
        const s = Module.medecompSockets[$0];
        if (!s || s.state !== 1) return 0;
        s.ws.send(UTF8ToString($1, $2));
        return 1;
    }, int(intptr_t(m_handle)), text.data(), int(text.size())) != 0;
}

bool WebSocket::recv(std::string* text, bool* closed) {
    *closed = false;
    if (!m_handle) {
        *closed = true;
        return false;
    }
    const int id = int(intptr_t(m_handle));
    int size = MAIN_THREAD_EM_ASM_INT({
        const s = Module.medecompSockets[$0];
        if (!s) return -2;
        if (s.queue.length) return lengthBytesUTF8(s.queue[0]);
        return s.state === 2 ? -2 : -1;
    }, id);
    if (size == -2) {
        *closed = true;
        return false;
    }
    if (size < 0) return false;
    std::string out(size_t(size) + 1, '\0');
    MAIN_THREAD_EM_ASM({
        const s = Module.medecompSockets[$0];
        stringToUTF8(s.queue.shift(), $1, $2);
    }, id, &out[0], size + 1);
    out.resize(size_t(size));
    *text = std::move(out);
    return true;
}

void WebSocket::close() {
    if (m_handle) {
        MAIN_THREAD_EM_ASM({
            const s = Module.medecompSockets && Module.medecompSockets[$0];
            if (s && s.ws) s.ws.close();
            if (s) delete Module.medecompSockets[$0];
        }, int(intptr_t(m_handle)));
    }
    m_handle = nullptr;
    m_partial.clear();
    m_pingAt = 0;
    m_rtt = -1;
}

#endif

}
}
