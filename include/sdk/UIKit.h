#pragma once

#include "sdk/CoreGraphics.h"
#include "sdk/Foundation.h"

class UIDevice : public NSObject {
public:
    static UIDevice* currentDevice();
    NSString* systemVersion();
    NSString* platformType();
    int orientation() const;
};

class UIApplication : public NSObject {
public:
    static UIApplication* sharedApplication();
    void setStatusBarOrientation(int orientation);
    void openURL(NSURL* url);
};

class UIImage : public NSObject {
public:
    static UIImage* imageWithCGImage(CGImageRef image);
    static UIImage* alloc();
    UIImage* initWithData(NSData* data);
    UIImage* autorelease();
    CGImageRef CGImage();
};

class UIViewController : public NSObject {};
class UIImagePickerController : public UIViewController {};

extern "C" void UIImageWriteToSavedPhotosAlbum(UIImage* image, void* target, SEL selector,
                                               void* contextInfo);
class UIFont : public NSObject {
public:
    static float systemFontSize();
    static UIFont* systemFontOfSize(float size);
    static UIFont* fontWithName_size(NSString* name, float size);
};

extern "C" void UIGraphicsPushContext(CGContext* context);
extern "C" void UIGraphicsPopContext();
