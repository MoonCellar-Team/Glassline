#pragma once

#include "sdk/UIKit.h"

class PhotoPicker;

class PhotoPickerImpl : public UIImagePickerController {
public:
    static PhotoPickerImpl* alloc();
    PhotoPickerImpl* initWithManager(PhotoPicker* manager);
};
