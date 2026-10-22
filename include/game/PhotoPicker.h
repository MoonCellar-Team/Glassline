#pragma once

#include "runtime/midp/Displayable.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Display;
class Image;
}
class PhotoPickerImpl;

class PhotoPicker : public midp::Displayable {
public:
    PhotoPicker();
    virtual ~PhotoPicker();
    virtual void showNotify();
    virtual void hideNotify();

    bool savePhoto(midp::Image* image);
    void deactivate();
    void photoCancel();
    void photoPicked(void* image);

    PhotoPickerImpl* m_impl;
    midp::Display* m_display;
    midp::ReferenceCountedPointer<midp::Displayable> m_previous;
    midp::ReferenceCountedPointer<midp::Image> m_photo;
    bool m_saveDone;
    bool m_saveFailed;
};
