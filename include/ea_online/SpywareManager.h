#pragma once

class SpywareManager {
public:
    static SpywareManager* getInstance();
    void trackFaithKilled();
    void trackCheckpointTriggered(int objectId);
    void trackGamePaused();
    void trackQuitLevel(int levelIndex);
    void trackResumeLevel(int levelIndex);
    void trackRestartLevel(int levelIndex);
    void trackLevelFinished(int levelIndex);
    void trackMultiplayerFinished();
    void trackStoryLevelStarted(int levelIndex);
    void trackRaceLevelStarted(int levelIndex);
    void trackMultiplayerStarted();
    void trackAchievedBadge(int idx);
    void trackGuardKilled(int objectId);
    void trackBagPickedUp(int objectId);
    void trackViewMoreGames();
    void trackViewAbout();
    void trackNewGame();
    void trackContinueGame();
    void trackOptionsSound();
    void trackViewBadges();
    void trackSelectMediaPicker();
    bool isEnabled();
    void setEnabled(bool enabled);
};
