#include "game/PhotoPicker.h"

#include <stddef.h>

#include "platform/PhotoPickerImpl.h"
#include "runtime/midp/Display.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/ReferenceCountedHardReference.h"

void PhotoPicker::hideNotify() {
    m_impl->performSelectorOnMainThread_withObject_waitUntilDone(sel_registerName("hide"), NULL, true);
}

void PhotoPicker::showNotify() {
    m_impl->performSelectorOnMainThread_withObject_waitUntilDone(sel_registerName("show"), NULL, true);
}

bool PhotoPicker::savePhoto(midp::Image* image) {
    midp::ReferenceCountedHardReference hold(image);
    UIImage* photo = UIImage::imageWithCGImage(static_cast<midp::ImageIPhone*>(image)->getCGImageRef());
    m_saveDone = false;
    UIImageWriteToSavedPhotosAlbum(photo, m_impl, sel_registerName("image:didFinishSavingWithError:contextInfo:"),
                                   NULL);
    return m_saveFailed;
}

void PhotoPicker::deactivate() {
    m_display->setCurrent(m_previous);
    m_previous = midp::ReferenceCountedPointer<midp::Displayable>(NULL);
}

void PhotoPicker::photoCancel() {
    m_photo = midp::ReferenceCountedPointer<midp::Image>(NULL);
    deactivate();
}

void PhotoPicker::photoPicked(void* image) {
    CGImageRef cgImage = static_cast<UIImage*>(image)->CGImage();
    m_photo = new midp::ImageIPhone(cgImage);
    deactivate();
}

PhotoPicker::~PhotoPicker() {
    if (m_impl != NULL) {
        m_impl->release();
    }
    midp::DECREF_NO_DELETE(this);
}

PhotoPicker::PhotoPicker()
    : m_impl(NULL), m_display(NULL), m_previous(NULL), m_photo(NULL), m_saveDone(false), m_saveFailed(false) {
    midp::INCREF(this);
    m_impl = PhotoPickerImpl::alloc()->initWithManager(this);
}
