#include "runtime/midp/Image.h"

#include <stddef.h>

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"
#include "runtime/midp/Thread.h"
#include "sdk/Foundation.h"
#include "sdk/UIKit.h"
#include "sdk/pthread.h"
#include "sdk/string.h"

namespace midp {

WebImage::WebImageResult WebImage::GetResult() {
    pthread_mutex_lock(&m_mutex);
    int result = m_result;
    pthread_mutex_unlock(&m_mutex);
    return (WebImageResult)result;
}

void WebImage::SetResult(WebImageResult result) {
    pthread_mutex_lock(&m_mutex);
    m_result = result;
    pthread_mutex_unlock(&m_mutex);
}

void WebImage::run() {
    INCREF(this);
    NSString* urlString = (NSString*)m_url->getCFString();
    NSURL* url = NSURL::URLWithString(urlString);
    NSData* data = NSData::dataWithContentsOfURL(url);
    UIImage* image = UIImage::alloc()->initWithData(data)->autorelease();
    setFrom(image->CGImage());
    SetResult(kSuccess);
    DECREF(this);
}

WebImage::~WebImage() {
    if (m_url) {
        m_url->~String();
    }
}

WebImage::WebImage(String* url) : ImageIPhone() {
    m_url = NULL;
    m_thread = pthread_t();
    m_result = 0;
    memset(&m_mutex, 0, sizeof(m_mutex));
    NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
    String* copy = new String(url);
    m_url = copy;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 2);
    pthread_create(&m_thread, &attr, Thread_WebImageRun, this);
    pthread_attr_destroy(&attr);
    pool->release();
}

}
