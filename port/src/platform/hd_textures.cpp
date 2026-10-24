#include "hd_textures.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../gfx/png.h"
#include "port/files.h"
#include "port/online.h"

#ifndef _WIN32
extern char** environ;
#endif

namespace port {
namespace platform {
namespace hd {

namespace {

#if defined(__linux__)
const char* const kToolUrl =
    "https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.5.0/realesrgan-ncnn-vulkan-20220424-ubuntu.zip";
const char* const kToolSha256 = "e5aa6eb131234b87c0c51f82b89390f5e3e642b7b70f2b9bbe95b6a285a40c96";
#elif defined(_WIN32)
const char* const kToolUrl =
    "https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.5.0/realesrgan-ncnn-vulkan-20220424-windows.zip";
const char* const kToolSha256 = "abc02804e17982a3be33675e4d471e91ea374e65b70167abc09e31acb412802d";
#else
const char* const kToolUrl = "";
const char* const kToolSha256 = "";
#endif
#ifdef _WIN32
const char* const kTool = "realesrgan-ncnn-vulkan.exe";
const char* const kToolDlls[] = {"vcomp140.dll"};
#else
const char* const kTool = "realesrgan-ncnn-vulkan";
#endif
const char* const kModelFiles[] = {"models/realesrgan-x4plus.param", "models/realesrgan-x4plus.bin"};
const int kPad = 16;
const int kScale = 4;
const int kMaxSide = 4096;

std::mutex s_mu;
Status s_status;
bool s_russian;
std::thread s_thread;
std::atomic<bool> s_stop(false);
#ifdef _WIN32
std::atomic<HANDLE> s_child(nullptr);
#else
std::atomic<pid_t> s_child(0);
#endif

const char* L(const char* ru, const char* en) { return s_russian ? ru : en; }

void setStage(const std::string& stage, int done = -1, int total = -1) {
    std::lock_guard<std::mutex> lock(s_mu);
    s_status.stage = stage;
    if (done >= 0) s_status.done = done;
    if (total >= 0) s_status.total = total;
}

bool fail(const std::string& error) {
    std::lock_guard<std::mutex> lock(s_mu);
    s_status.error = error;
    return false;
}

bool makeDirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); i++)
        if (i == path.size() || path[i] == '/')
            if (mkdir(path.substr(0, i).c_str(), 0755) != 0 && errno != EEXIST) return false;
    return true;
}

std::string sha256(const std::string& data) {
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::string msg = data;
    uint64_t bits = uint64_t(data.size()) * 8;
    msg += char(0x80);
    while (msg.size() % 64 != 56) msg += char(0);
    for (int i = 7; i >= 0; i--) msg += char(bits >> (i * 8));
    auto rotr = [](uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
    for (size_t off = 0; off < msg.size(); off += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(&msg[off + i * 4]);
            w[i] = uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
        }
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = hh + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh = g, g = f, f = e, e = d + t1, d = c, c = b, b = a, a = t1 + t2;
        }
        h[0] += a, h[1] += b, h[2] += c, h[3] += d, h[4] += e, h[5] += f, h[6] += g, h[7] += hh;
    }
    char out[65];
    for (int i = 0; i < 8; i++) snprintf(out + i * 8, 9, "%08x", h[i]);
    return out;
}

uint32_t le32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | uint32_t(p[3]) << 24; }
uint16_t le16(const unsigned char* p) { return uint16_t(p[0] | p[1] << 8); }

bool unzipFile(const std::string& zip, const std::string& name, std::string* out) {
    const unsigned char* z = reinterpret_cast<const unsigned char*>(zip.data());
    size_t n = zip.size();
    size_t eocd = std::string::npos;
    for (size_t i = n >= 22 ? n - 22 : 0; i + 22 <= n && i != std::string::npos; i--) {
        if (le32(z + i) == 0x06054b50) {
            eocd = i;
            break;
        }
        if (i == 0) break;
    }
    if (eocd == std::string::npos) return false;
    size_t pos = le32(z + eocd + 16), count = le16(z + eocd + 10);
    for (size_t e = 0; e < count && pos + 46 <= n; e++) {
        if (le32(z + pos) != 0x02014b50) return false;
        int method = le16(z + pos + 10);
        size_t packed = le32(z + pos + 20), size = le32(z + pos + 24);
        size_t nameLen = le16(z + pos + 28), extra = le16(z + pos + 30), comment = le16(z + pos + 32);
        size_t local = le32(z + pos + 42);
        std::string entry(reinterpret_cast<const char*>(z + pos + 46), std::min(nameLen, n - pos - 46));
        pos += 46 + nameLen + extra + comment;
        if (entry != name) continue;
        if (local + 30 > n) return false;
        size_t data = local + 30 + le16(z + local + 26) + le16(z + local + 28);
        if (data + packed > n) return false;
        if (method == 0) {
            out->assign(zip, data, packed);
            return true;
        }
        if (method != 8) return false;
        out->resize(size);
        z_stream s = {};
        if (inflateInit2(&s, -MAX_WBITS) != Z_OK) return false;
        s.next_in = const_cast<Bytef*>(z + data);
        s.avail_in = uInt(packed);
        s.next_out = reinterpret_cast<Bytef*>(&(*out)[0]);
        s.avail_out = uInt(size);
        int r = inflate(&s, Z_FINISH);
        inflateEnd(&s);
        return r == Z_STREAM_END && s.total_out == size;
    }
    return false;
}

bool writeFile(const std::string& path, const std::string& data, mode_t mode) {
    std::string tmp = path + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    ok = fclose(f) == 0 && ok;
    if (ok) chmod(tmp.c_str(), mode);
    if (!ok || !files::replaceFile(tmp, path)) {
        remove(tmp.c_str());
        return false;
    }
    return true;
}

std::string toolDir() { return files::dataDir() + "/tools/realesrgan"; }

bool ensureTool() {
    std::string dir = toolDir();
    if (files::stat(dir + "/" + kTool).exists && files::stat(dir + "/" + kModelFiles[1]).exists) return true;
    if (!*kToolUrl) return fail(L("на этой системе увеличение пока не поддерживается", "upscaling is not supported on this system yet"));
    setStage(L("Скачиваю Real-ESRGAN (47 МБ)…", "Downloading Real-ESRGAN (47 MB)…"));
    online::Response r = online::request("GET", kToolUrl, "", {}, 600000);
    if (r.status != 200) return fail(std::string(L("не скачать Real-ESRGAN: ", "cannot download Real-ESRGAN: ")) +
                                     (r.error.empty() ? std::to_string(r.status) : r.error));
    if (sha256(r.body) != kToolSha256) return fail(L("скачанный Real-ESRGAN не сходится по SHA-256 — не запускаю",
                                                    "downloaded Real-ESRGAN fails the SHA-256 check — not running it"));
    if (!makeDirs(dir + "/models")) return fail(L("не создать ", "cannot create ") + dir);
    std::string data;
    for (const char* name : kModelFiles)
        if (!unzipFile(r.body, name, &data) || !writeFile(dir + "/" + name, data, 0644))
            return fail(std::string(L("не распаковать ", "cannot unpack ")) + name);
    if (!unzipFile(r.body, kTool, &data) || !writeFile(dir + "/" + kTool, data, 0755))
        return fail(std::string(L("не распаковать ", "cannot unpack ")) + kTool);
#ifdef _WIN32
    for (const char* name : kToolDlls)
        if (!unzipFile(r.body, name, &data) || !writeFile(dir + "/" + name, data, 0644))
            return fail(std::string(L("не распаковать ", "cannot unpack ")) + name);
#endif
    return true;
}

#ifdef _WIN32
std::wstring wide(const std::string& utf8) {
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring out(n > 0 ? size_t(n - 1) : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &out[0], n);
    return out;
}

std::wstring quoted(const std::wstring& arg) {
    std::wstring out = L"\"";
    size_t slashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') {
            slashes++;
            continue;
        }
        out.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        slashes = 0;
        out += c;
    }
    out.append(slashes * 2, L'\\');
    return out + L"\"";
}

bool run(const std::vector<std::string>& args, const std::vector<std::string>& env = {}) {
    std::wstring cmd;
    for (const std::string& a : args) cmd += (cmd.empty() ? L"" : L" ") + quoted(wide(a));
    std::wstring block;
    if (wchar_t* current = GetEnvironmentStringsW()) {
        for (wchar_t* e = current; *e; e += wcslen(e) + 1) block.append(e, wcslen(e) + 1);
        FreeEnvironmentStringsW(current);
    }
    for (const std::string& e : env) block += wide(e) + L'\0';
    block += L'\0';
    SECURITY_ATTRIBUTES inherit = {sizeof inherit, nullptr, TRUE};
    HANDLE null = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &inherit, OPEN_EXISTING, 0, nullptr);
    STARTUPINFOW si = {};
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = si.hStdError = null;
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                             &block[0], nullptr, &si, &pi);
    if (null != INVALID_HANDLE_VALUE) CloseHandle(null);
    if (!ok) return false;
    CloseHandle(pi.hThread);
    s_child = pi.hProcess;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    s_child = nullptr;
    CloseHandle(pi.hProcess);
    return code == 0;
}
#else
bool run(const std::vector<std::string>& args, const std::vector<std::string>& env = {}) {
    std::vector<char*> argv;
    for (const std::string& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    std::vector<std::string> envStrings;
    for (char** e = environ; *e; e++) envStrings.push_back(*e);
    for (const std::string& e : env) envStrings.push_back(e);
    std::vector<char*> envp;
    for (std::string& e : envStrings) envp.push_back(&e[0]);
    envp.push_back(nullptr);
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, 1, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0);
    pid_t pid = 0;
    int err = posix_spawn(&pid, argv[0], &actions, nullptr, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&actions);
    if (err != 0) return false;
    s_child = pid;
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    s_child = 0;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif

std::string selfPath() {
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    return n > 0 ? std::string(buf, size_t(n)) : std::string();
}

bool ensureDump(const std::string& game) {
    std::string dump = files::dataDir() + "/textures/dump";
    for (const std::string& name : files::list(dump))
        if (name.size() > 4 && name.compare(name.size() - 4, 4, ".png") == 0) return true;
    setStage(L("Выгружаю текстуры игры…", "Dumping the game textures…"));
    std::string self = selfPath();
    if (self.empty()) return fail(L("не найти свой исполняемый файл", "cannot find own executable"));
    if (run({self, "--dump-textures", game}, {"SDL_VIDEODRIVER=offscreen"}) || run({self, "--dump-textures", game}))
        return true;
    return fail(L("выгрузка текстур не удалась (medecomp --dump-textures)", "texture dump failed (medecomp --dump-textures)"));
}

void bleed(gfx::Image& img) {
    int w = img.width, h = img.height;
    std::vector<uint8_t> known(size_t(w) * h);
    for (size_t i = 0; i < known.size(); i++) known[i] = img.rgba[i * 4 + 3] > 8;
    for (int pass = 0; pass < 32; pass++) {
        std::vector<uint8_t> next = known;
        bool changed = false;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                size_t i = size_t(y) * w + x;
                if (known[i]) continue;
                int sum[3] = {0, 0, 0}, n = 0;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int xx = x + dx, yy = y + dy;
                        if (xx < 0 || yy < 0 || xx >= w || yy >= h || !known[size_t(yy) * w + xx]) continue;
                        const uint8_t* p = &img.rgba[(size_t(yy) * w + xx) * 4];
                        sum[0] += p[0], sum[1] += p[1], sum[2] += p[2], n++;
                    }
                if (!n) continue;
                for (int c = 0; c < 3; c++) img.rgba[i * 4 + c] = uint8_t(sum[c] / n);
                next[i] = 1;
                changed = true;
            }
        known.swap(next);
        if (!changed) break;
    }
}

gfx::Image padded(const gfx::Image& img, int channel) {
    gfx::Image out;
    out.width = img.width + 2 * kPad;
    out.height = img.height + 2 * kPad;
    out.rgba.resize(size_t(out.width) * out.height * 4);
    for (int y = 0; y < out.height; y++)
        for (int x = 0; x < out.width; x++) {
            int sx = ((x - kPad) % img.width + img.width) % img.width;
            int sy = ((y - kPad) % img.height + img.height) % img.height;
            const uint8_t* s = &img.rgba[(size_t(sy) * img.width + sx) * 4];
            uint8_t* d = &out.rgba[(size_t(y) * out.width + x) * 4];
            if (channel < 0) d[0] = s[0], d[1] = s[1], d[2] = s[2];
            else d[0] = d[1] = d[2] = s[channel];
            d[3] = 255;
        }
    return out;
}

bool readPNG(const std::string& path, gfx::Image* img) {
    std::vector<unsigned char> bytes;
    return files::read(path, &bytes) && gfx::decodePNG(bytes.data(), bytes.size(), *img, false);
}

bool upscale(const gfx::Image& in, const std::string& work, gfx::Image* out) {
    std::string src = work + "/in.png", dst = work + "/out.png";
    remove(dst.c_str());
    if (!gfx::writePNG(src, in.width, in.height, in.rgba.data())) return false;
    std::string dir = toolDir();
    if (!run({dir + "/" + kTool, "-i", src, "-o", dst, "-n", "realesrgan-x4plus", "-s", "4", "-f", "png", "-m",
              dir + "/models"}))
        return false;
    gfx::Image big;
    if (!readPNG(dst, &big) || big.width != in.width * kScale || big.height != in.height * kScale) return false;
    int cut = kPad * kScale;
    out->width = big.width - 2 * cut;
    out->height = big.height - 2 * cut;
    out->rgba.resize(size_t(out->width) * out->height * 4);
    for (int y = 0; y < out->height; y++)
        memcpy(&out->rgba[size_t(y) * out->width * 4], &big.rgba[(size_t(y + cut) * big.width + cut) * 4],
               size_t(out->width) * 4);
    return true;
}

bool processOne(const std::string& name, const std::string& work) {
    std::string root = files::dataDir() + "/textures";
    gfx::Image img;
    if (!readPNG(root + "/dump/" + name, &img)) return fail(name + L(": не прочитать", ": cannot read"));
    bool alpha = false;
    for (size_t i = 3; i < img.rgba.size(); i += 4)
        if (img.rgba[i] < 255) {
            alpha = true;
            break;
        }
    if (alpha) bleed(img);
    gfx::Image color, mask;
    if (!upscale(padded(img, -1), work, &color))
        return fail(name + L(": Real-ESRGAN не справился (нужна видеокарта с Vulkan)",
                             ": Real-ESRGAN failed (a Vulkan GPU is needed)"));
    if (alpha) {
        if (!upscale(padded(img, 3), work, &mask)) return fail(name + L(": альфа не увеличилась", ": alpha upscale failed"));
        for (size_t i = 0; i < color.rgba.size(); i += 4) color.rgba[i + 3] = mask.rgba[i];
    }
    while (color.width > kMaxSide || color.height > kMaxSide) {
        gfx::Image half;
        half.width = color.width / 2;
        half.height = color.height / 2;
        half.rgba.resize(size_t(half.width) * half.height * 4);
        for (int y = 0; y < half.height; y++)
            for (int x = 0; x < half.width; x++)
                for (int c = 0; c < 4; c++) {
                    int s = 0;
                    for (int k = 0; k < 4; k++)
                        s += color.rgba[(size_t(y * 2 + k / 2) * color.width + x * 2 + k % 2) * 4 + c];
                    half.rgba[(size_t(y) * half.width + x) * 4 + c] = uint8_t(s / 4);
                }
        color = std::move(half);
    }
    std::string target = root + "/" + name, tmp = target + ".tmp.png";
    if (!gfx::writePNG(tmp, color.width, color.height, color.rgba.data()) || !files::replaceFile(tmp, target)) {
        remove(tmp.c_str());
        return fail(name + L(": не записать", ": cannot write"));
    }
    return true;
}

void job(std::string game) {
    auto started = std::chrono::steady_clock::now();
    if (!ensureTool() || s_stop || !ensureDump(game) || s_stop) {
        std::lock_guard<std::mutex> lock(s_mu);
        s_status.running = false;
        return;
    }
    std::string root = files::dataDir() + "/textures", work = root + "/.work";
    makeDirs(work);
    std::vector<std::string> todo;
    int total = 0;
    for (const std::string& name : files::list(root + "/dump")) {
        if (name.size() <= 4 || name.compare(name.size() - 4, 4, ".png") != 0) continue;
        total++;
        if (!files::stat(root + "/" + name).exists) todo.push_back(name);
    }
    std::sort(todo.begin(), todo.end());
    int done = total - int(todo.size()), made = 0, failed = 0;
    for (const std::string& name : todo) {
        if (s_stop) break;
        double spent = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        std::string eta;
        if (made > 0) {
            int left = int(spent / made * (todo.size() - made) / 60 + 1);
            eta = std::string(L(", осталось ~", ", ~")) + std::to_string(left) + L(" мин", " min left");
        }
        setStage(std::string(L("Увеличиваю ", "Upscaling ")) + name + eta, done, total);
        bool ok = processOne(name, work);
        if (s_stop) break;
        ok ? made++ : failed++;
        done++;
        setStage("", done, total);
    }
    remove((work + "/in.png").c_str());
    remove((work + "/out.png").c_str());
    rmdir(work.c_str());
    std::lock_guard<std::mutex> lock(s_mu);
    s_status.running = false;
    if (s_stop) s_status.error.clear();
    s_status.stage = s_stop ? L("Остановлено — продолжится со следующей текстуры", "Stopped — will continue from the next texture")
                            : failed ? L("Готово, с ошибками", "Done, with errors")
                                     : L("Готово: включите «Свои текстуры»", "Done: turn on “Custom textures”");
}

}

void start(const std::string& game, bool russian) {
    {
        std::lock_guard<std::mutex> lock(s_mu);
        if (s_status.running) return;
        s_status = Status();
        s_status.running = true;
    }
    s_russian = russian;
    if (s_thread.joinable()) s_thread.join();
    s_stop = false;
    s_thread = std::thread(job, game);
}

void stop(bool wait) {
    s_stop = true;
#ifdef _WIN32
    if (HANDLE child = s_child.load()) TerminateProcess(child, 1);
#else
    if (pid_t pid = s_child.load()) kill(pid, SIGTERM);
#endif
    if (wait && s_thread.joinable()) s_thread.join();
}

Status status() {
    std::lock_guard<std::mutex> lock(s_mu);
    return s_status;
}

}
}
}
