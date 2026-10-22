#pragma once

#include "sdk/UIKit.h"

class MediaPicker;

class MediaPickerImpl : public UIViewController {
public:
    static MediaPickerImpl* alloc();
    MediaPickerImpl* initWithManager(MediaPicker* manager);
};
