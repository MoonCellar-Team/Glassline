#pragma once

namespace midp {
class Image;
class String;
}

namespace ea {

class EASpywareManager {
public:
    static EASpywareManager* getInstance();
    void logEvent(int event);
    bool isEnabled() const;
    void setEnabled(bool enabled);
    void setLanguage(midp::String* locale);
    void logLanguageChange(midp::String* locale);
    void retryTickers();
    void appStarted(bool firstLaunch);
    void hideOrigin();
    void showOrigin(int);
    void showMoreGames();
    int getTickerCount();
    midp::String* getTickerString(int index);
    midp::String* getTickerURL(int index);
    void MTXpause();
    void MTXresume();
    midp::String* getBannerURL();
    midp::Image* getBannerImage();
};

}
