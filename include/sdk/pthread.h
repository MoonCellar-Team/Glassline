#pragma once

extern "C" {

struct _opaque_pthread_mutex_t {
    long __sig;
    char __opaque[40];
};
struct _opaque_pthread_mutexattr_t {
    long __sig;
    char __opaque[8];
};
typedef struct _opaque_pthread_mutex_t pthread_mutex_t;
typedef struct _opaque_pthread_mutexattr_t pthread_mutexattr_t;

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

struct _opaque_pthread_t;
typedef struct _opaque_pthread_t* pthread_t;

struct _opaque_pthread_attr_t {
    long __sig;
    char __opaque[36];
};
typedef struct _opaque_pthread_attr_t pthread_attr_t;

int pthread_attr_init(pthread_attr_t* attr);
int pthread_attr_destroy(pthread_attr_t* attr);
int pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate);
int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start)(void*), void* arg);

}
