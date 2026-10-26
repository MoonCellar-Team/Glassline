#pragma once

#include <stdint.h>

#include <functional>
#include <string>
#include <vector>

namespace port {
namespace online {

struct Response {
    int status = 0;
    std::string body;
    std::string error;
};

bool available(std::string* why = nullptr);
Response request(const std::string& method, const std::string& url, const std::string& body = std::string(),
                 const std::vector<std::string>& headers = std::vector<std::string>(), long timeoutMs = 15000);

bool download(const std::string& url, const std::string& path,
              const std::function<bool(long long done, long long total)>& progress, std::string* error,
              bool localHttp = false);

std::string serverUrl();
std::string token();
std::string userName();
void saveAccount(const std::string& token, const std::string& userName);
std::string deviceName();

struct Account {
    std::string id, userName, avatar;
};

struct DeviceCode {
    std::string deviceCode;
    std::string userCode;
    std::string url;
    std::string urlWithCode;
    int expiresIn = 0;
    int interval = 5;
};

bool startSignIn(const std::string& server, const std::string& device, DeviceCode* out, std::string* error);
enum class SignIn { Pending, Approved, Failed };
SignIn pollSignIn(const std::string& server, const std::string& deviceCode, std::string* token, Account* account,
                  std::string* error);
bool fetchAccount(const std::string& server, const std::string& token, Account* out, bool* revoked,
                  std::string* error);
void signOut(const std::string& server, const std::string& token);

class WebSocket {
public:
    WebSocket() = default;
    ~WebSocket() { close(); }
    WebSocket(const WebSocket&) = delete;
    WebSocket& operator=(const WebSocket&) = delete;

    bool open(const std::string& url, const std::vector<std::string>& headers, std::string* error);
    bool send(const std::string& text);
    bool recv(std::string* text, bool* closed);
    void close();
    bool isOpen() const { return m_handle != nullptr; }
    bool ping();
    int rttMs() const { return m_rtt; }

private:
    void* m_handle = nullptr;
    void* m_headers = nullptr;
    std::string m_partial;
    long long m_pingAt = 0;
    int m_rtt = -1;
};

void frame();

void startCheck();

int consoleSignIn();
int consoleSignOut();

bool qrCode(const std::string& text, int* width, std::vector<bool>* modules);
bool decodeImage(const std::string& bytes, int* width, int* height, std::vector<uint8_t>* rgba);

}
}
