#include "ea_online/SpywareManager.h"
#include "ea_online/ea/EASpywareManager.h"

#include <stddef.h>

namespace ea {

static bool s_enabled = true;

EASpywareManager* EASpywareManager::getInstance() {
    static EASpywareManager instance;
    return &instance;
}

bool EASpywareManager::isEnabled() const { return s_enabled; }
void EASpywareManager::setEnabled(bool enabled) { s_enabled = enabled; }

void EASpywareManager::logEvent(int) {}
void EASpywareManager::setLanguage(midp::String*) {}
void EASpywareManager::logLanguageChange(midp::String*) {}
void EASpywareManager::appStarted(bool) {}
void EASpywareManager::retryTickers() {}
void EASpywareManager::hideOrigin() {}
void EASpywareManager::showOrigin(int) {}
void EASpywareManager::showMoreGames() {}
void EASpywareManager::MTXpause() {}
void EASpywareManager::MTXresume() {}

int EASpywareManager::getTickerCount() { return 0; }
midp::String* EASpywareManager::getTickerString(int) { return NULL; }
midp::String* EASpywareManager::getTickerURL(int) { return NULL; }
midp::String* EASpywareManager::getBannerURL() { return NULL; }
midp::Image* EASpywareManager::getBannerImage() { return NULL; }

}

SpywareManager* SpywareManager::getInstance() {
    static SpywareManager instance;
    return &instance;
}

bool SpywareManager::isEnabled() { return ea::EASpywareManager::getInstance()->isEnabled(); }
void SpywareManager::setEnabled(bool enabled) { ea::EASpywareManager::getInstance()->setEnabled(enabled); }

void SpywareManager::trackFaithKilled() {}
void SpywareManager::trackCheckpointTriggered(int) {}
void SpywareManager::trackGamePaused() {}
void SpywareManager::trackQuitLevel(int) {}
void SpywareManager::trackResumeLevel(int) {}
void SpywareManager::trackRestartLevel(int) {}
void SpywareManager::trackLevelFinished(int) {}
void SpywareManager::trackMultiplayerFinished() {}
void SpywareManager::trackStoryLevelStarted(int) {}
void SpywareManager::trackRaceLevelStarted(int) {}
void SpywareManager::trackMultiplayerStarted() {}
void SpywareManager::trackAchievedBadge(int) {}
void SpywareManager::trackGuardKilled(int) {}
void SpywareManager::trackBagPickedUp(int) {}
void SpywareManager::trackViewMoreGames() {}
void SpywareManager::trackViewAbout() {}
void SpywareManager::trackNewGame() {}
void SpywareManager::trackContinueGame() {}
void SpywareManager::trackOptionsSound() {}
void SpywareManager::trackViewBadges() {}
void SpywareManager::trackSelectMediaPicker() {}
