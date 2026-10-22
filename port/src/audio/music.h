#pragma once

#include <string>
#include <vector>

namespace port {
namespace audio {

bool musicSupported();
std::vector<std::string> musicTracks(const std::string& folder);
bool ownMusicPlaying();

bool musicSetting();
void setMusicSetting(bool on);
std::string musicFolderSetting();
void setMusicFolderSetting(const std::string& folder);
bool musicShuffleSetting();
void setMusicShuffleSetting(bool on);
int musicVolumeSetting();
void setMusicVolumeSetting(int percent);

int volumeSetting();
void setVolumeSetting(int percent);

bool backgroundPauseSetting();
void setBackgroundPauseSetting(bool pause);

}
}
