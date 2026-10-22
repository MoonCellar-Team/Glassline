#pragma once

#include "runtime/midp/Displayable.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Display;
}
class MediaPickerImpl;

class MediaPicker : public midp::Displayable {
public:
    MediaPicker();
    virtual ~MediaPicker();
    virtual void showNotify();
    virtual void hideNotify();

    void activate(midp::Display* display);
    void deactivate();
    void mediaCancel();
    void mediaPicked(void* unused);

    MediaPickerImpl* m_impl;
    midp::Display* m_display;
    midp::ReferenceCountedPointer<midp::Displayable> m_previous;
    bool m_active;
    bool m_unk21;
    bool m_unk22;
};
