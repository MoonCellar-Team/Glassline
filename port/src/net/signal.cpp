#include <string>

#include "../ea/json.h"
#include "port/net.h"
#include "port/online.h"

namespace port {
namespace net {

namespace {

port::online::WebSocket s_socket;

SignalTurn turnOf(const port::online::Json& json) {
    SignalTurn turn;
    for (const port::online::Json& url : json["urls"].items) turn.servers.push_back(url.str());
    turn.user = json["username"].str();
    turn.pass = json["credential"].str();
    return turn;
}

SignalPeer peerOf(const port::online::Json& json) {
    SignalPeer peer;
    peer.name = json["name"].str();
    peer.address = json["address"].str();
    peer.port = uint16_t(json["port"].integer());
    return peer;
}

std::string signalUrl() {
    std::string url = port::online::serverUrl();
    if (url.compare(0, 5, "https") == 0) url = "wss" + url.substr(5);
    else if (url.compare(0, 4, "http") == 0) url = "ws" + url.substr(4);
    return url + "/signal";
}

}

std::string SignalPeer::endpoint() const {
    bool v6 = address.find(':') != std::string::npos;
    return (v6 ? "[" + address + "]" : address) + ":" + std::to_string(port);
}

bool signalOpen(std::string* error) {
    std::vector<std::string> headers;
    std::string token = port::online::token();
    if (!token.empty()) headers.push_back("Authorization: Bearer " + token);
    return s_socket.open(signalUrl(), headers, error);
}

bool signalHost(uint16_t port) {
    return s_socket.send("{\"type\":\"host\",\"port\":" + std::to_string(port) + "}");
}

bool signalJoin(const std::string& code, uint16_t port) {
    return s_socket.send("{\"type\":\"join\",\"code\":" + port::online::jsonString(code) +
                         ",\"port\":" + std::to_string(port) + "}");
}

bool signalSend(const std::string& data) {
    return s_socket.send("{\"type\":\"signal\",\"data\":" + port::online::jsonString(data) + "}");
}

bool signalPoll(SignalReceived* out) {
    if (!s_socket.isOpen()) return false;
    std::string text;
    bool closed = false;
    while (s_socket.recv(&text, &closed)) {
        port::online::Json json;
        if (!port::online::parseJson(text, &json)) continue;
        std::string type = json["type"].str();
        *out = SignalReceived();
        if (type == "hosted") {
            out->event = SignalEvent::Hosted;
            out->code = json["code"].str();
            out->turn = turnOf(json["turn"]);
        } else if (type == "joined" || type == "peer") {
            out->event = type == "joined" ? SignalEvent::Joined : SignalEvent::Peer;
            out->code = json["code"].str();
            out->peer = peerOf(json["peer"]);
            out->turn = turnOf(json["turn"]);
        } else if (type == "signal") {
            out->event = SignalEvent::Message;
            out->text = json["data"].str();
        } else if (type == "left") {
            out->event = SignalEvent::Left;
        } else if (type == "closed") {
            out->event = SignalEvent::Closed;
        } else if (type == "error") {
            out->event = SignalEvent::Error;
            out->text = json["message"].str();
        } else {
            continue;
        }
        return true;
    }
    if (closed) {
        s_socket.close();
        *out = SignalReceived();
        out->event = SignalEvent::Closed;
        out->text = "соединение с сервером закрыто";
        return true;
    }
    return false;
}

void signalClose() {
    if (s_socket.isOpen()) s_socket.send("{\"type\":\"leave\"}");
    s_socket.close();
}

}
}
