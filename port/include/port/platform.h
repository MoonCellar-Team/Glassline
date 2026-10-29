#pragma once

#include <functional>
#include <string>
#include <vector>

struct SDL_Window;

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#define MEDECOMP_ONE_PROCESS 1
#endif

namespace midp {
class Display;
class GLES;
}

namespace port {
namespace platform {

const int kViewWidth = 768;
const int kViewHeight = 1024;
const int kScreenWidth = 1024;
const int kScreenHeight = 768;

struct View {
    int glesMajor = 2;
    int glesMinor = 0;
    midp::GLES* gl = nullptr;
    midp::Display* display = nullptr;
};
View& view();

void bind();
void flush();
void draw();

bool createWindow(const char* title);
unsigned framebuffer();
void present();
void destroyWindow();
void acquireContext();
void yieldContext();
void releaseContext();
bool inBackground();
void surfaceDestroying();
bool toScreen(float windowX, float windowY, int* x, int* y);
void toFrame(float windowX, float windowY, float* x, float* y);
void windowSize(int* w, int* h);
void warpPointer(int x, int y);
void toggleFullscreen();
enum class WindowMode { Window = 0, Fullscreen = 1, Borderless = 2 };
WindowMode windowModeSetting();
void setWindowModeSetting(WindowMode mode);
enum class VideoDriver { Auto = 0, Wayland = 1, X11 = 2 };
bool gamescopeSession();
VideoDriver videoDriverSetting();
void setVideoDriverSetting(VideoDriver driver);
void applyVideoDriver(bool reinit);
float displayScale(SDL_Window* window, int drawableWidth);
const int kFrameRateDisplay = -1;
const int kFrameRateUnlimited = 0;
int frameRateSetting();
void setFrameRateSetting(int fps);
bool vsyncSetting();
void setVsyncSetting(bool on);
int displaySetting();
void setDisplaySetting(int display);
bool resolutionSetting(int* width, int* height);
void setResolutionSetting(int width, int height);
int displayRefreshRate(int display);
std::vector<int> frameRateChoices(int display);
int frameTimerRate();
unsigned long presentCount();
enum class Upscale { Fsr = 0, Linear = 1, Nearest = 2 };
Upscale upscaleSetting();
float sharpnessSetting();
void setUpscaleSetting(Upscale mode, float sharpness);
int wideExtra();
int framebufferHeight();
bool widescreenSetting();
void setWidescreenSetting(bool on);
float renderScale();
int renderSetting();
void setRenderSetting(int percent);
int textureFilterSetting();
void setTextureFilterSetting(int level);
int msaaSetting();
void setMsaaSetting(int samples);
int postSetting();
float postStrength();
void setPostSetting(int mode, float strength);
int ssaoSetting();
void setSsaoSetting(int mode);
bool godRaysSetting();
void setGodRaysSetting(bool on);
bool sunInCamera(float out[3]);
bool sunInWorld(float out[3]);
bool motionBlurSetting();
void setMotionBlurSetting(bool on);
float ssrSetting();
float ssrStrengthSetting();
void setSsrSetting(bool on, float strength);
#ifdef _WIN32
bool underWine();
bool nativeShaderCompiler();
#else
inline bool underWine() { return false; }
inline bool nativeShaderCompiler() { return true; }
#endif
bool lensFlareSetting();
void setLensFlareSetting(bool on);
int gradeSetting();
void setGradeSetting(int preset);
bool taaSetting();
void setTaaSetting(bool on);
bool gameCamera(float cameraToWorld[16], float* heroineSpeed);
bool originalGraphics();
void setOriginalGraphics(bool on);
int framebufferPixelWidth();
int framebufferPixelHeight();
void debugFrame(int dt);
void perfFrame(double workMs);
void requestScreenshot(const std::string& path);
void requestFrameShot(const std::string& path);
bool takeScriptFinish();
#ifdef __EMSCRIPTEN__
void webStartup();
void webGameSelected();
SDL_Window* webTakeLauncherWindow();
void webGameStep(std::function<bool()> step);
void webGameFrame();
#endif
#ifdef __ANDROID__
void androidStartup();
int androidDisplayRotation();
std::string androidPickFile(const std::string& dir, const char* mime);
void androidRestart();
std::string androidSaveToDownloads(const std::string& path, const std::string& name, const char* mime);
#endif
#ifdef MEDECOMP_ONE_PROCESS
bool takeEditorRequest(std::string* id);
void appRestart();
#endif
bool presentSplash();
int takeScriptAchievement();
void setWindowTitle(const std::string& title);
void startTextInput();
void stopTextInput();
void openURL(const char* url);
void wakeMainLoop();

void runTimers();
int msUntilNextTimer();

bool textInputActive();
void textInput(const char* utf8);
void textKey(int key);
const int kKeyBackspace = 8;
const int kKeyReturn = 13;
const int kKeyEscape = 27;
bool textLineActive();
bool textFieldImage(std::vector<unsigned char>* rgba, int* width, int* height);
void presentTextField();

enum MenuFocusCmd { MenuFocusLeft = 1, MenuFocusRight, MenuFocusUp, MenuFocusDown, MenuFocusActivate, MenuFocusBack };
void menuFocusPost(int cmd);
void menuFocusPointer();
void menuFocusScroll(int px, int steps);
bool menuFocusActive();
void menuFocusApply();
const void* menuFocusTarget();
void menuFocusSet(const void* target);

void setTilt(float tilt);
bool preciseInput();
float tilt();
void runAccelerometer();
void accelerate(float tilt);
#ifdef _WIN32
void windowsStartup();
#endif

}
}
