#include "update.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <zlib.h>

#include "../ea/json.h"
#include "monocypher-ed25519.h"
#include "port/brand.h"
#include "port/online.h"
#include "settings.h"

namespace port {
namespace platform {
namespace update {

namespace {

std::mutex s_mu;
Status s_status;
bool s_started = false, s_flagOff = false;
std::string s_version = "dev";
std::vector<std::string> s_args;
std::string s_asset, s_assetUrl, s_sigUrl, s_target;
bool s_zip = false;

const char* tr(bool russian, const char* ru, const char* en) { return russian ? ru : en; }

void set(const Status& st) {
    std::lock_guard<std::mutex> lock(s_mu);
    s_status = st;
}

bool parseVersion(std::string v, int out[3]) {
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) v.erase(0, 1);
    out[0] = out[1] = out[2] = 0;
    int part = 0;
    size_t i = 0;
    while (part < 3) {
        if (i >= v.size() || v[i] < '0' || v[i] > '9') return false;
        long n = 0;
        while (i < v.size() && v[i] >= '0' && v[i] <= '9' && n < 100000) n = n * 10 + (v[i++] - '0');
        out[part++] = int(n);
        if (i == v.size()) return part >= 2;
        if (v[i] != '.') return false;
        i++;
    }
    return i == v.size();
}

std::string plainNotes(const std::string& md) {
    std::string out;
    size_t i = 0;
    while (i < md.size()) {
        if (md.compare(i, 2, "**") == 0) {
            i += 2;
            continue;
        }
        if (md[i] == '[') {
            size_t close = md.find("](", i);
            size_t end = close == std::string::npos ? std::string::npos : md.find(')', close);
            size_t line = md.find('\n', i);
            if (close != std::string::npos && end != std::string::npos && (line == std::string::npos || end < line)) {
                out += md.substr(i + 1, close - i - 1);
                i = end + 1;
                continue;
            }
        }
        if ((i == 0 || md[i - 1] == '\n') && md.compare(i, 2, "- ") == 0) {
            out += "• ";
            i += 2;
            continue;
        }
        if (md[i] != '\r') out += md[i];
        i++;
    }
    while (!out.empty() && (out.back() == '\n' || out.back() == ' ')) out.pop_back();
    return out;
}

#ifdef _WIN32
std::wstring wide(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(n > 0 ? size_t(n) : 1, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], n);
    out.resize(n > 0 ? size_t(n - 1) : 0);
    return out;
}
std::string narrow(const std::wstring& s) {
    int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? size_t(n) : 1, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, &out[0], n, nullptr, nullptr);
    out.resize(n > 0 ? size_t(n - 1) : 0);
    return out;
}
FILE* openFile(const std::string& path, const char* mode) { return _wfopen(wide(path).c_str(), wide(mode).c_str()); }
bool removeFile(const std::string& path) { return DeleteFileW(wide(path).c_str()) != 0; }
bool renameFile(const std::string& from, const std::string& to) {
    return MoveFileExW(wide(from).c_str(), wide(to).c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
}
bool makeDir(const std::string& path) { return CreateDirectoryW(wide(path).c_str(), nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS; }
bool exists(const std::string& path) { return GetFileAttributesW(wide(path).c_str()) != INVALID_FILE_ATTRIBUTES; }
std::string exeDir() {
    wchar_t buf[4096];
    DWORD n = GetModuleFileNameW(nullptr, buf, DWORD(sizeof buf / sizeof buf[0]));
    std::string path = n ? narrow(std::wstring(buf, n)) : std::string();
    size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}
#else
FILE* openFile(const std::string& path, const char* mode) { return fopen(path.c_str(), mode); }
bool removeFile(const std::string& path) { return remove(path.c_str()) == 0; }
bool renameFile(const std::string& from, const std::string& to) { return rename(from.c_str(), to.c_str()) == 0; }
bool makeDir(const std::string& path) {
    struct stat st;
    return mkdir(path.c_str(), 0755) == 0 || (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}
bool exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}
#endif

std::string dirOf(const std::string& path) {
    size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
}

bool writable(const std::string& dir) {
    const std::string probe = dir + "/.glassline-update-probe";
    FILE* f = openFile(probe, "wb");
    if (!f) return false;
    fclose(f);
    removeFile(probe);
    return true;
}

bool readAll(const std::string& path, std::string* out) {
    FILE* f = openFile(path, "rb");
    if (!f) return false;
    out->clear();
    char buf[1 << 16];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) out->append(buf, n);
    bool ok = !ferror(f);
    fclose(f);
    return ok;
}

bool writeAll(const std::string& path, const std::string& data) {
    FILE* f = openFile(path, "wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    return fclose(f) == 0 && ok;
}

bool parseKey(const std::string& hex, uint8_t key[32]) {
    if (hex.size() != 64) return false;
    for (int i = 0; i < 32; i++) {
        unsigned v = 0;
        if (sscanf(hex.c_str() + 2 * i, "%2x", &v) != 1) return false;
        key[i] = uint8_t(v);
    }
    return true;
}

std::string keyHex() {
    const char* test = getenv("MEDECOMP_UPDATE_KEY");
    if (test && *test && getenv("MEDECOMP_UPDATE_URL")) return test;
    return MEDECOMP_UPDATE_KEY;
}

bool verify(const std::string& path, const std::string& name, const std::string& sig) {
    uint8_t key[32];
    if (sig.size() != 64 || !parseKey(keyHex(), key)) return false;
    FILE* f = openFile(path, "rb");
    if (!f) return false;
    crypto_sha512_ctx ctx;
    crypto_sha512_init(&ctx);
    std::vector<uint8_t> buf(1 << 20);
    size_t n;
    while ((n = fread(buf.data(), 1, buf.size(), f)) > 0) crypto_sha512_update(&ctx, buf.data(), n);
    bool readOk = !ferror(f);
    fclose(f);
    uint8_t hash[64];
    crypto_sha512_final(&ctx, hash);
    if (!readOk) return false;
    char hex[129];
    for (int i = 0; i < 64; i++) snprintf(hex + 2 * i, 3, "%02x", hash[i]);
    const std::string line = std::string(hex) + "  " + name + "\n";
    return crypto_ed25519_check(reinterpret_cast<const uint8_t*>(sig.data()), key, reinterpret_cast<const uint8_t*>(line.data()),
                                line.size()) == 0;
}

uint32_t le32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | uint32_t(p[3]) << 24; }
uint16_t le16(const unsigned char* p) { return uint16_t(p[0] | p[1] << 8); }

struct Entry {
    std::string name;
    std::string data;
};

bool readZip(const std::string& zip, std::vector<Entry>* out) {
    const unsigned char* z = reinterpret_cast<const unsigned char*>(zip.data());
    const size_t n = zip.size();
    if (n < 22) return false;
    size_t eocd = std::string::npos;
    for (size_t i = n - 22;; i--) {
        if (le32(z + i) == 0x06054b50) {
            eocd = i;
            break;
        }
        if (i == 0 || n - i > 22 + 65535) break;
    }
    if (eocd == std::string::npos) return false;
    size_t pos = le32(z + eocd + 16);
    const size_t count = le16(z + eocd + 10);
    std::string root;
    for (size_t e = 0; e < count; e++) {
        if (pos + 46 > n || le32(z + pos) != 0x02014b50) return false;
        const int method = le16(z + pos + 10);
        const size_t packed = le32(z + pos + 20), size = le32(z + pos + 24);
        const size_t nameLen = le16(z + pos + 28), extra = le16(z + pos + 30), comment = le16(z + pos + 32);
        const size_t local = le32(z + pos + 42);
        if (pos + 46 + nameLen > n) return false;
        std::string name(reinterpret_cast<const char*>(z + pos + 46), nameLen);
        pos += 46 + nameLen + extra + comment;
        for (char& c : name)
            if (c == '\\') c = '/';
        if (name.empty() || name.back() == '/') continue;
        const size_t slash = name.find('/');
        if (slash == std::string::npos) return false;
        if (root.empty()) root = name.substr(0, slash + 1);
        if (name.compare(0, root.size(), root) != 0) return false;
        name.erase(0, root.size());
        if (name.empty() || name[0] == '/' || name.find("..") != std::string::npos || name.find(':') != std::string::npos) return false;
        if (local + 30 > n) return false;
        const size_t data = local + 30 + le16(z + local + 26) + le16(z + local + 28);
        if (data + packed > n) return false;
        Entry entry;
        entry.name = name;
        if (method == 0) {
            entry.data.assign(zip, data, packed);
        } else if (method == 8) {
            entry.data.resize(size);
            z_stream s = {};
            if (inflateInit2(&s, -MAX_WBITS) != Z_OK) return false;
            s.next_in = const_cast<Bytef*>(z + data);
            s.avail_in = uInt(packed);
            s.next_out = reinterpret_cast<Bytef*>(size ? &entry.data[0] : nullptr);
            s.avail_out = uInt(size);
            const int r = inflate(&s, Z_FINISH);
            inflateEnd(&s);
            if (r != Z_STREAM_END || s.total_out != size) return false;
        } else {
            return false;
        }
        out->push_back(std::move(entry));
    }
    return !out->empty();
}

bool installZip(const std::string& zipPath, const std::string& dir, std::string* why) {
    std::string zip;
    std::vector<Entry> entries;
    if (!readAll(zipPath, &zip) || !readZip(zip, &entries)) return *why = "zip", false;
    zip.clear();
    for (const Entry& e : entries) {
        std::string path = dir;
        size_t start = 0, slash;
        while ((slash = e.name.find('/', start)) != std::string::npos) {
            path += "/" + e.name.substr(start, slash - start);
            if (!makeDir(path)) return *why = path, false;
            start = slash + 1;
        }
        if (!writeAll(dir + "/" + e.name + ".new", e.data)) return *why = e.name, false;
    }
    std::vector<std::string> moved;
    for (const Entry& e : entries) {
        const std::string path = dir + "/" + e.name;
        const bool had = exists(path);
        if (had) {
            removeFile(path + ".old");
            if (!renameFile(path, path + ".old")) {
                *why = e.name;
                for (const std::string& m : moved) {
                    renameFile(m, m + ".new");
                    renameFile(m + ".old", m);
                }
                return false;
            }
        }
        if (!renameFile(path + ".new", path)) {
            *why = e.name;
            if (had) renameFile(path + ".old", path);
            return false;
        }
        moved.push_back(path);
    }
    return true;
}

std::string releasesUrl() {
    if (const char* url = getenv("MEDECOMP_UPDATE_URL"); url && *url) return url;
    return "https://api.github.com/repos/" MEDECOMP_PUBLIC_REPO "/releases/latest";
}

void findTarget() {
    if (const char* t = getenv("MEDECOMP_UPDATE_TARGET"); t && *t) {
        const std::string v = t;
        if (v.compare(0, 9, "appimage:") == 0) s_target = v.substr(9), s_zip = false;
        if (v.compare(0, 4, "zip:") == 0) s_target = v.substr(4), s_zip = true;
        return;
    }
#ifdef _WIN32
    s_target = exeDir(), s_zip = true;
#elif !defined(__ANDROID__)
    if (const char* appimage = getenv("APPIMAGE"); appimage && *appimage) s_target = appimage, s_zip = false;
#endif
}

bool latestWithoutApi(std::string* version, std::string* notes) {
    const port::online::Response c =
        port::online::request("GET", "https://raw.githubusercontent.com/" MEDECOMP_PUBLIC_REPO "/main/CHANGELOG.md", std::string(), {}, 15000);
    if (c.status != 200) return false;
    const size_t head = c.body.find("\n## ");
    if (head == std::string::npos) return false;
    const size_t lineEnd = c.body.find('\n', head + 1);
    const std::string line = c.body.substr(head + 4, lineEnd == std::string::npos ? std::string::npos : lineEnd - head - 4);
    *version = line.substr(0, line.find(' '));
    const size_t next = lineEnd == std::string::npos ? std::string::npos : c.body.find("\n## ", lineEnd);
    *notes = lineEnd == std::string::npos ? std::string() : c.body.substr(lineEnd + 1, next == std::string::npos ? std::string::npos : next - lineEnd - 1);
    return !version->empty();
}

void runCheck(bool russian) {
    Status st;
    st.stage = Stage::Checking;
    set(st);
    findTarget();
    std::string name = MEDECOMP_PORT_NAME;
    for (char& c : name) c = char(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    auto assetName = [&](const std::string& v, bool zip) {
        return name + (zip ? "-windows-x64-" + v + ".zip" : "-linux-x64-" + v + ".AppImage");
    };
    std::string version, notes, page, assetUrl, sigUrl;
    const port::online::Response r =
        port::online::request("GET", releasesUrl(), std::string(), {"Accept: application/vnd.github+json"}, 15000);
    port::online::Json j;
    if (r.status == 200 && port::online::parseJson(r.body, &j)) {
        version = j["tag_name"].str();
        if (!version.empty() && (version[0] == 'v' || version[0] == 'V')) version.erase(0, 1);
        notes = j["body"].str();
        page = j["html_url"].str("https://github.com/" MEDECOMP_PUBLIC_REPO "/releases");
        for (const port::online::Json& a : j["assets"].items) {
            if (a["name"].str() == assetName(version, s_zip)) assetUrl = a["browser_download_url"].str();
            if (a["name"].str() == assetName(version, s_zip) + ".sig") sigUrl = a["browser_download_url"].str();
        }
    } else if ((r.status == 403 || r.status == 429) && !getenv("MEDECOMP_UPDATE_URL") && latestWithoutApi(&version, &notes)) {
        fprintf(stderr, "medecomp: обновление: API GitHub — HTTP %d (лимит запросов), выпуск — по CHANGELOG.md\n", r.status);
        const std::string base = "https://github.com/" MEDECOMP_PUBLIC_REPO "/releases/download/v" + version + "/";
        page = "https://github.com/" MEDECOMP_PUBLIC_REPO "/releases/tag/v" + version;
        assetUrl = base + assetName(version, s_zip);
        sigUrl = assetUrl + ".sig";
        if (compareVersions(version, s_version) > 0) {
            const std::string probe = s_target.empty() ? base + assetName(version, false) + ".sig" : sigUrl;
            const port::online::Response sig = port::online::request("GET", probe, std::string(), {}, 15000);
            if (sig.status != 200 || sig.body.size() != 64) {
                st.stage = Stage::Current;
                set(st);
                fprintf(stderr, "medecomp: обновление: %s в CHANGELOG.md, но выпуск ещё не выложен\n", version.c_str());
                return;
            }
        }
    } else {
        st.stage = r.status == 404 ? Stage::Current : Stage::Failed;
        st.error = r.error.empty() ? "HTTP " + std::to_string(r.status) : r.error;
        set(st);
        fprintf(stderr, "medecomp: обновление: выпуск не узнать — %s\n", st.error.c_str());
        return;
    }
    if (compareVersions(version, s_version) <= 0) {
        st.stage = Stage::Current;
        set(st);
        fprintf(stderr, "medecomp: обновление: %s — последняя (выпуск %s)\n", s_version.c_str(), version.c_str());
        return;
    }
    st.version = version;
    st.notes = plainNotes(notes);
    st.page = page;
    const std::string want = assetName(version, s_zip);
    uint8_t key[32];
    const bool keyOk = parseKey(keyHex(), key);
    const std::string where = s_zip ? s_target : dirOf(s_target);
    st.installable = !s_target.empty() && !assetUrl.empty() && !sigUrl.empty() && keyOk && writable(where);
    fprintf(stderr, "medecomp: обновление: есть %s (это %s); %s — %s\n", version.c_str(), s_version.c_str(), want.c_str(),
            st.installable              ? "можно поставить"
            : s_target.empty()          ? "эту установку не обновить, только скачать"
            : assetUrl.empty()          ? "файла нет в выпуске"
            : sigUrl.empty() || !keyOk ? "подписи нет"
                                        : "в папку не записать");
    {
        std::lock_guard<std::mutex> lock(s_mu);
        s_asset = want, s_assetUrl = assetUrl, s_sigUrl = sigUrl;
    }
    st.stage = Stage::Available;
    set(st);
    (void)russian;
}

void runInstall(bool russian, Status st) {
    std::string asset, assetUrl, sigUrl;
    {
        std::lock_guard<std::mutex> lock(s_mu);
        asset = s_asset, assetUrl = s_assetUrl, sigUrl = s_sigUrl;
    }
    auto fail = [&](const std::string& why) {
        st.stage = Stage::Failed;
        st.error = why;
        set(st);
        fprintf(stderr, "medecomp: обновление: %s\n", why.c_str());
    };
    const bool local = getenv("MEDECOMP_UPDATE_URL") != nullptr;
    const port::online::Response sig = port::online::request("GET", sigUrl, std::string(), {}, 30000);
    if (sig.status != 200 || sig.body.size() != 64)
        return fail(tr(russian, "не скачать подпись обновления", "could not download the update's signature"));
    const std::string file = s_zip ? s_target + "/" + asset + ".download" : s_target + ".download";
    st.stage = Stage::Downloading;
    set(st);
    std::string error;
    const bool got = port::online::download(
        assetUrl, file,
        [&](long long done, long long total) {
            std::lock_guard<std::mutex> lock(s_mu);
            s_status.done = done, s_status.total = total;
            return true;
        },
        &error, local);
    if (!got) {
        removeFile(file);
        return fail(std::string(tr(russian, "не скачать обновление: ", "could not download the update: ")) + error);
    }
    st.stage = Stage::Installing;
    set(st);
    if (!verify(file, asset, sig.body)) {
        removeFile(file);
        return fail(tr(russian, "подпись обновления не сходится — оно не поставлено", "the update's signature does not match — it was not installed"));
    }
    if (s_zip) {
        std::string why;
        const bool ok = installZip(file, s_target, &why);
        removeFile(file);
        if (!ok) return fail(std::string(tr(russian, "не заменить файлы игры: ", "could not replace the game's files: ")) + why);
    } else {
#ifndef _WIN32
        chmod(file.c_str(), 0755);
#endif
        if (!renameFile(file, s_target)) {
            removeFile(file);
            return fail(std::string(tr(russian, "не заменить ", "could not replace ")) + s_target);
        }
    }
    fprintf(stderr, "medecomp: обновление: %s поставлено, подпись сошлась\n", st.version.c_str());
    st.stage = Stage::Ready;
    set(st);
}

}

int compareVersions(const std::string& a, const std::string& b) {
    int x[3], y[3];
    if (!parseVersion(a, x) || !parseVersion(b, y)) return 0;
    for (int i = 0; i < 3; i++)
        if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
    return 0;
}

void setup(int argc, char** argv, const char* version) {
    s_args.assign(argv, argv + argc);
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--no-update") == 0) s_flagOff = true;
    if (version && *version) s_version = version;
}

std::string currentVersion() { return s_version; }

bool enabled() {
    int v[3];
    if (s_flagOff || !parseVersion(s_version, v)) return false;
    if (getenv("FLATPAK_ID")) return false;
    return port::platform::settings::get("check_updates", "on") != "off";
}

void check(bool russian) {
    {
        std::lock_guard<std::mutex> lock(s_mu);
        if (s_started || !enabled()) return;
        s_started = true;
        s_status.stage = Stage::Checking;
    }
    std::thread(runCheck, russian).detach();
}

void install(bool russian) {
    Status st;
    {
        std::lock_guard<std::mutex> lock(s_mu);
        const bool retry = s_status.stage == Stage::Failed && !s_status.version.empty();
        if ((s_status.stage != Stage::Available && !retry) || !s_status.installable) return;
        s_status.stage = Stage::Downloading;
        s_status.done = s_status.total = 0;
        s_status.error.clear();
        st = s_status;
    }
    std::thread(runInstall, russian, st).detach();
}

Status status() {
    std::lock_guard<std::mutex> lock(s_mu);
    return s_status;
}

bool restart() {
    if (status().stage != Stage::Ready || s_target.empty()) return false;
#ifdef _WIN32
    std::string exe;
    {
        wchar_t buf[4096];
        DWORD n = GetModuleFileNameW(nullptr, buf, DWORD(sizeof buf / sizeof buf[0]));
        exe = n ? narrow(std::wstring(buf, n)) : std::string();
    }
    std::wstring line = L"\"" + wide(exe) + L"\"";
    for (size_t i = 1; i < s_args.size(); i++) line += L" \"" + wide(s_args[i]) + L"\"";
    STARTUPINFOW si = {};
    si.cb = sizeof si;
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(wide(exe).c_str(), &line[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    if (s_zip) return false;
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(s_target.c_str()));
    for (size_t i = 1; i < s_args.size(); i++) argv.push_back(const_cast<char*>(s_args[i].c_str()));
    argv.push_back(nullptr);
    fprintf(stderr, "medecomp: обновление: перезапуск — %s\n", s_target.c_str());
    fflush(stderr);
    execv(s_target.c_str(), argv.data());
    return false;
#endif
}

void cleanup() {
#ifdef _WIN32
    std::thread([] {
        const std::string dir = exeDir();
        for (int attempt = 0; attempt < 30; attempt++) {
            int left = 0;
            std::vector<std::string> dirs{dir};
            while (!dirs.empty()) {
                const std::string d = dirs.back();
                dirs.pop_back();
                WIN32_FIND_DATAW f;
                HANDLE h = FindFirstFileW(wide(d + "\\*").c_str(), &f);
                if (h == INVALID_HANDLE_VALUE) continue;
                do {
                    const std::string name = narrow(f.cFileName);
                    if (name == "." || name == "..") continue;
                    const std::string path = d + "\\" + name;
                    if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) dirs.push_back(path);
                    else if ((name.size() > 4 && name.compare(name.size() - 4, 4, ".old") == 0) ||
                             (name.size() > 9 && name.compare(name.size() - 9, 9, ".download") == 0))
                        left += !removeFile(path);
                } while (FindNextFileW(h, &f));
                FindClose(h);
            }
            if (!left) return;
            Sleep(500);
        }
    }).detach();
#endif
}

}
}
}
