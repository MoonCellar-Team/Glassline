#pragma once

class GameCenter {
public:
    GameCenter();
    ~GameCenter();
    void authenticateLocalPlayer();
    bool isGameCenterAPIAvailable();
};
