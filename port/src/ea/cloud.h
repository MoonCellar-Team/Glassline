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
bool skinsActive();
enum class SkinPlace { Off, Here, New, Changed, Saved };
SkinPlace skinPlace(const std::string& id);
int skinsForCloud();
bool placeNewSkin(const std::string& id);
bool sendSkin(const std::string& id, bool russian);
void keepSkinHere(const std::string& id, bool russian);
std::string newSkinId();
void syncSkins(bool russian);
std::string skinsNotice();

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
