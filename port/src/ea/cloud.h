#pragma once

#include <string>

namespace port {
namespace online {
namespace cloud {

bool enabled();
void setEnabled(bool on);
bool skinsEnabled();
void setSkinsEnabled(bool on);
constexpr int kSkinLimit = 10;
int customSkins();
bool skinLimitActive();

void startSync(bool russian, bool afterGame = false, bool withSave = true);
void syncLevels();
std::string sha256(const std::string& data);
bool conflictPending(long long* localTime, long long* remoteTime);
void resolve(bool takeRemote, bool russian);
bool busy();
std::string status();
void waitSync(int timeoutMs);
void syncNow(bool russian);
bool takeSkinsChanged();
bool applyDownloadedPrefs();

bool allowed();
void markPlaying();
bool interrupted();
void progress(int* done, int* total);

}
}
}
