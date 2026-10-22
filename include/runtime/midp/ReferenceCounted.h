#pragma once

namespace midp {

class ReferenceCounted {
public:
    ReferenceCounted();
    virtual ~ReferenceCounted();
    virtual bool _decRef();
    virtual void _incRef();
    int getRefCount() const;

    volatile int m_refCount;
};

void INCREF(ReferenceCounted* obj);
bool DECREF(ReferenceCounted* obj);
void DECREF_NO_DELETE(ReferenceCounted* obj);

}
