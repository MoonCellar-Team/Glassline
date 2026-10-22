#include "process.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace port {
namespace platform {
namespace process {

#ifdef _WIN32
namespace {

std::wstring wide(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(n > 0 ? n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], n);
    return out;
}

std::wstring quoted(const std::wstring& a) {
    std::wstring out = L"\"";
    size_t slashes = 0;
    for (wchar_t c : a) {
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

}

std::string selfPath() {
    wchar_t buf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, buf, DWORD(sizeof buf / sizeof buf[0]));
    if (n == 0) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, buf, int(n), nullptr, 0, nullptr, nullptr);
    std::string out(size_t(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, buf, int(n), &out[0], len, nullptr, nullptr);
    return out;
}

long spawn(const std::vector<std::string>& args, const std::vector<std::string>& env) {
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
    if (!ok) return 0;
    CloseHandle(pi.hThread);
    return long(reinterpret_cast<intptr_t>(pi.hProcess));
}

int poll(long id) {
    HANDLE h = reinterpret_cast<HANDLE>(intptr_t(id));
    if (WaitForSingleObject(h, 0) == WAIT_TIMEOUT) return -1;
    DWORD code = 1;
    GetExitCodeProcess(h, &code);
    CloseHandle(h);
    return int(code);
}

#else
std::string selfPath() {
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    return n > 0 ? std::string(buf, size_t(n)) : std::string();
}

namespace {

long start(const std::vector<std::string>& args, const std::vector<std::string>& env, bool search) {
#ifdef __ANDROID__
    (void)args, (void)env, (void)search;
    return 0;
#endif
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
    int err = search ? posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), envp.data())
                     : posix_spawn(&pid, argv[0], &actions, nullptr, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&actions);
    return err == 0 ? long(pid) : 0;
}

}

long spawn(const std::vector<std::string>& args, const std::vector<std::string>& env) { return start(args, env, false); }

int poll(long id) {
    int status = 0;
    pid_t done = waitpid(pid_t(id), &status, WNOHANG);
    if (done == 0 || (done < 0 && errno == EINTR)) return -1;
    if (done < 0) return 1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

#endif

}
}
}
