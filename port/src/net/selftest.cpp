#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <string>

#include "port/net.h"

namespace port {
namespace net {

namespace {

long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::string text(const Received& r) { return std::string(r.data.begin(), r.data.end()); }

int runHost(int port) {
    std::string error;
    if (!host(uint16_t(port), &error)) {
        fprintf(stderr, "сеть: %s\n", error.c_str());
        return 1;
    }
    printf("сеть: жду на порту %d\n", port);
    fflush(stdout);
    long long deadline = nowMs() + 30000;
    int echoed = 0;
    bool met = false;
    while (nowMs() < deadline) {
        Received r;
        while (poll(&r)) {
            if (r.event == Event::Connected) {
                met = true;
                deadline = nowMs() + 30000;
                printf("сеть: подключился %s\n", r.text.c_str());
            } else if (r.event == Event::Message) {
                std::string reply = "e" + text(r);
                send(r.peer, reply.data(), reply.size(), !r.data.empty() && r.data[0] == 'n');
                echoed++;
            } else if (r.event == Event::Disconnected) {
                printf("сеть: пир отключился (%s), эхо — %d\n", r.text.c_str(), echoed);
                shutdown();
                return met ? 0 : 1;
            }
        }
        usleep(2000);
    }
    printf("сеть: никто не подключился\n");
    shutdown();
    return 1;
}

int runClient(const std::string& address) {
    std::string error;
    Peer peer = connect(address, &error);
    if (!peer) {
        fprintf(stderr, "сеть: %s\n", error.c_str());
        return 1;
    }
    const int kReliable = 5, kUnreliable = 40;
    int reliableBack = 0, unreliableBack = 0;
    bool connected = false, sent = false;
    long long deadline = nowMs() + 10000, finish = 0;
    while (nowMs() < deadline) {
        Received r;
        while (poll(&r)) {
            if (r.event == Event::Connected) {
                connected = true;
                printf("сеть: подключено к %s\n", address.c_str());
            } else if (r.event == Event::Message) {
                if (r.data.size() >= 2 && r.data[1] == 'n') reliableBack++;
                else unreliableBack++;
            } else if (r.event == Event::Disconnected) {
                fprintf(stderr, "сеть: соединение закрыто: %s\n", r.text.c_str());
                deadline = 0;
            }
        }
        if (connected && !sent) {
            for (int i = 0; i < kReliable; i++) {
                std::string m = "n" + std::to_string(i);
                send(peer, m.data(), m.size(), true);
            }
            for (int i = 0; i < kUnreliable; i++) {
                std::string m = "b" + std::to_string(i);
                send(peer, m.data(), m.size(), false);
            }
            sent = true;
            finish = nowMs() + 1500;
        }
        if (sent && (reliableBack == kReliable && unreliableBack == kUnreliable)) break;
        if (sent && nowMs() > finish && reliableBack == kReliable) break;
        usleep(2000);
    }
    int ms = ping(peer);
    printf("сеть: надёжных вернулось %d из %d, ненадёжных — %d из %d, пинг %d мс\n", reliableBack, kReliable,
           unreliableBack, kUnreliable, ms);
    disconnect(peer);
    for (int i = 0; i < 50; i++) {
        Received r;
        while (poll(&r)) {
        }
        usleep(10000);
    }
    shutdown();
    return connected && reliableBack == kReliable && unreliableBack > 0 ? 0 : 1;
}

int runRoomHost() {
    if (!sessionHost()) {
        fprintf(stderr, "сеть: %s\n", sessionErrorName(sessionInfo().error));
        return 1;
    }
    long long deadline = nowMs() + 120000;
    bool printed = false, met = false;
    int echoed = 0;
    while (nowMs() < deadline) {
        sessionFrame();
        SessionInfo info = sessionInfo();
        if (!printed && !info.code.empty()) {
            printf("сеть: комната %s\n", info.code.c_str());
            fflush(stdout);
            printed = true;
        }
        Received r;
        while (sessionPoll(&r)) {
            if (r.event == Event::Connected) {
                met = true;
                printf("сеть: соединение с %s (%s)\n", info.peerName.c_str(), r.text.c_str());
                fflush(stdout);
            } else if (r.event == Event::Message) {
                std::string reply = "\x1f" + text(r);
                sessionSend(reply.data(), reply.size(), !r.data.empty() && r.data[0] == 0x10);
                echoed++;
            }
        }
        if (info.state == SessionState::Failed) {
            printf("сеть: %s, эхо — %d\n", sessionErrorName(info.error), echoed);
            sessionClose();
            shutdown();
            return met ? 0 : 1;
        }
        usleep(2000);
    }
    printf("сеть: никто не подключился\n");
    sessionClose();
    shutdown();
    return 1;
}

int runRoomJoin(const std::string& code) {
    if (!sessionJoin(code)) {
        fprintf(stderr, "сеть: %s\n", sessionErrorName(sessionInfo().error));
        return 1;
    }
    const int kReliable = 5, kUnreliable = 40;
    int reliableBack = 0, unreliableBack = 0;
    bool connected = false, sent = false, sentUnreliable = false;
    long long deadline = nowMs() + 40000, finish = 0;
    while (nowMs() < deadline) {
        sessionFrame();
        SessionInfo info = sessionInfo();
        if (info.state == SessionState::Failed) {
            fprintf(stderr, "сеть: %s\n", sessionErrorName(info.error));
            break;
        }
        Received r;
        while (sessionPoll(&r)) {
            if (r.event == Event::Connected) {
                connected = true;
                printf("сеть: соединение с %s через P2P (%s)\n", info.peerName.c_str(), r.text.c_str());
                fflush(stdout);
            } else if (r.event == Event::Message) {
                if (r.data.size() >= 2 && r.data[1] == 0x10) reliableBack++;
                else unreliableBack++;
            }
        }
        if (connected && !sent) {
            for (int i = 0; i < kReliable; i++) {
                std::string m = "\x10" + std::to_string(i);
                sessionSend(m.data(), m.size(), true);
            }
            sent = true;
        }
        if (sent && !sentUnreliable && reliableBack > 0) {
            for (int i = 0; i < kUnreliable; i++) {
                std::string m = "\x11" + std::to_string(i);
                sessionSend(m.data(), m.size(), false);
            }
            sentUnreliable = true;
            finish = nowMs() + 1500;
        }
        if (sent && reliableBack == kReliable && unreliableBack == kUnreliable) break;
        if (sentUnreliable && nowMs() > finish && reliableBack == kReliable) break;
        usleep(2000);
    }
    int ms = sessionInfo().ping;
    printf("сеть: надёжных вернулось %d из %d, ненадёжных — %d из %d, пинг %d мс\n", reliableBack, kReliable,
           unreliableBack, kUnreliable, ms);
    sessionClose();
    for (int i = 0; i < 50; i++) {
        Received r;
        while (poll(&r)) {
        }
        usleep(10000);
    }
    shutdown();
    return connected && reliableBack == kReliable && unreliableBack > 0 ? 0 : 1;
}

}

int selfTest(bool hostMode, const std::string& argument) {
    std::string why;
    if (!available(&why)) {
        fprintf(stderr, "сеть недоступна: %s\n", why.c_str());
        return 2;
    }
    return hostMode ? runHost(atoi(argument.c_str())) : runClient(argument);
}

int selfTestRoom(bool hostMode, const std::string& argument) {
    std::string why;
    if (!available(&why)) {
        fprintf(stderr, "сеть недоступна: %s\n", why.c_str());
        return 2;
    }
    return hostMode ? runRoomHost() : runRoomJoin(argument);
}

}
}
