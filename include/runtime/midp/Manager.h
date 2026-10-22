#pragma once

namespace midp {

class Player;
class String;

class Manager {
public:
    static Player* createPlayer(String* locator);
    static Player* createDuplicatePlayer(Player* player);
    static bool isOtherAudioPlaying();
    static void setListenerPosition(float x, float y, float z);
    static void setListenerVelocity(float x, float y, float z);
    static void setListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ);
    static void update();
};

}
