#include "runtime/midp/array.h"

#include <stddef.h>

void* operator new(size_t);
void operator delete(void*) noexcept;
#include <new>

extern "C" {
void __Unwind_SjLj_Register(void*);
void __Unwind_SjLj_Unregister(void*);
void __Unwind_SjLj_Resume(void*);
}

namespace midp {

template<class T>
static T* array_at_4(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 4);
}

template<class T>
static T* array_at_104(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x68);
}

template<class T>
static T* array_at_20(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x14);
}

template<class T>
static void* array_discard_s20(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 0x14;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void array_d0(array<T>* self) {
    self->~array();
    ::operator delete(self);
}

template<class T>
static void array_d1(array<T>* self) {
    self->~array();
}

template<class T>
static void array_c1_default(array<T>* self) {
    new (self) array<T>();
}

template<class T>
static void array_c1_copy(array<T>* self, const array<T>& other) {
    new (self) array<T>(other);
}

template<class T>
static void array_c1_int(array<T>* self, int length) {
    new (self) array<T>(length);
}

template<class T>
static void array_ctor_tmp_0(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x14));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x14) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x14, length, true));
}

template<class T>
static void* array_discard_s4(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 4;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void array_fill_1(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 1));
    for (int i = 0; i < length; i++) {
        unsigned char* dst = buf + i * 1;
        *dst = 0;
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 1, length, true));
}

template<class T>
static void array_ctor_tmp_2(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 4));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 4) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 4, length, true));
}

template<class T>
static T* array_at_304(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x130);
}

template<class T>
static T* array_at_2(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 2);
}

template<class T>
static T* array_at_1(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i);
}

template<class T>
static void array_fill_3(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 2));
    for (int i = 0; i < length; i++) {
        unsigned char* dst = buf + i * 2;
        *reinterpret_cast<short*>(dst) = 0;
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 2, length, true));
}

template<class T>
static T* array_at_52(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x34);
}

template<>
array<int>::array(const int* data, int length) : array_base(0, NULL) {
    if (length < 1 || data == NULL)
        return;
    void* raw = ::operator new(sizeof(array_data));
    setLengthAndData(length, new (raw) array_data(const_cast<int*>(data), 4, length, false));
}

void array_c1_ptr(array<int>* self, const int* data, int length) {
    new (self) array<int>(data, length);
}

template<class T>
static void* array_discard_s104(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 0x68;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void* array_discard_s52(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 0x34;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void array_ctor_tmp_4(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x68));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x68) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x68, length, true));
}

template<class T>
static void array_ctor_tmp_5(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x34));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x34) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x34, length, true));
}

template<class T>
static T* array_at_16(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x10);
}

template<class T>
static T* array_at_24(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x18);
}

template<class T>
static void array_fill_6(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x10));
    for (int i = 0; i < length; i++) {
        unsigned char* dst = buf + i * 0x10;
        for (int b = 0; b < 0x10; b += 4)
            *reinterpret_cast<int*>(dst + b) = 0;
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x10, length, true));
}

template<class T>
static void array_fill_7(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x18));
    for (int i = 0; i < length; i++) {
        unsigned char* dst = buf + i * 0x18;
        for (int b = 0; b < 0x18; b += 4)
            *reinterpret_cast<int*>(dst + b) = 0;
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x18, length, true));
}

template<class T>
static T* array_at_28(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x1c);
}

template<class T>
static void* array_discard_s28(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 0x1c;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void array_ctor_tmp_8(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x1c));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x1c) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x1c, length, true));
}

template<class T>
static T* array_at_40(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x28);
}

template<class T>
static T* array_at_36(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x24);
}

template<class T>
static T* array_at_68(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x44);
}

template<class T>
static void* array_discard_s40(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 0x28;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void array_ctor_tmp_9(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x1c));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x1c) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x1c, length, true));
}

template<class T>
static void array_ctor_tmp_10(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x28));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x28) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x28, length, true));
}

template<class T>
static void array_ctor_tmp_11(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x24));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x24) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x24, length, true));
}

template<class T>
static void array_ctor_tmp_12(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x44));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x44) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x44, length, true));
}

template<class T>
static T* array_at_32(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x20);
}

template<class T>
static void* array_discard_s32(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 0x20;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void array_ctor_tmp_13(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x20));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x20) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x20, length, true));
}

template<class T>
static void array_dtor_sjlj_14(array<T>* self) {
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    self->discard();
    self->array_base::~array_base();
    ::operator delete(self);
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static void array_dtor_sjlj_15(array<T>* self) {
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    self->discard();
    self->array_base::~array_base();
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static void array_ctor_sjlj_16(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    if (length >= 1) {
        unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 4));
        for (int i = 0; i < length; i++) {
            T tmp;
            new (buf + i * 4) T(tmp);
        }
        void* raw = ::operator new(sizeof(array_data));
        self->setLengthAndData(length, new (raw) array_data(buf, 4, length, true));
    }
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static void array_dtor_sjlj_17(array<T>* self) {
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    self->discard();
    self->array_base::~array_base();
    ::operator delete(self);
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static void array_dtor_sjlj_18(array<T>* self) {
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    self->discard();
    self->array_base::~array_base();
    ::operator delete(self);
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static void array_dtor_sjlj_19(array<T>* self) {
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    self->discard();
    self->array_base::~array_base();
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static void array_dtor_sjlj_20(array<T>* self) {
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    self->discard();
    self->array_base::~array_base();
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static void array_ctor_sjlj_21(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    if (length >= 1) {
        unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x14));
        for (int i = 0; i < length; i++) {
            T tmp;
            new (buf + i * 0x14) T(tmp);
        }
        void* raw = ::operator new(sizeof(array_data));
        self->setLengthAndData(length, new (raw) array_data(buf, 0x14, length, true));
    }
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static void array_ctor_sjlj_22(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    unsigned char frame[160];
    __Unwind_SjLj_Register(frame);
    if (length >= 1) {
        unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x14));
        for (int i = 0; i < length; i++) {
            T tmp;
            new (buf + i * 0x14) T(tmp);
        }
        void* raw = ::operator new(sizeof(array_data));
        self->setLengthAndData(length, new (raw) array_data(buf, 0x14, length, true));
    }
    __Unwind_SjLj_Unregister(frame);
}

template<class T>
static T* array_at_56(array<T>* self, int i) {
    return reinterpret_cast<T*>(static_cast<char*>(self->data()) + i * 0x38);
}

template<class T>
static void* array_discard_s304(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 0x130;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void* array_discard_s56(array<T>* self) {
    int count = self->m_length;
    void* p = self->array_base::discard();
    if (p != NULL) {
        if (count >= 1) {
            unsigned char* it = static_cast<unsigned char*>(p);
            do {
                reinterpret_cast<T*>(it)->~T();
                it += 0x38;
                count -= 1;
            } while (count != 0);
        }
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
static void array_ctor_tmp_23(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x130));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x130) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x130, length, true));
}

template<class T>
static void array_ctor_tmp_24(array<T>* self, int length) {
    new (self) array_base(0, NULL);
    if (length < 1)
        return;
    unsigned char* buf = static_cast<unsigned char*>(::operator new((size_t)length * 0x38));
    for (int i = 0; i < length; i++) {
        T tmp;
        new (buf + i * 0x38) T(tmp);
    }
    void* raw = ::operator new(sizeof(array_data));
    self->setLengthAndData(length, new (raw) array_data(buf, 0x38, length, true));
}

}
