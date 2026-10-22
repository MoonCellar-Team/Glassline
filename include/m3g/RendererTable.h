#pragma once
#include "runtime/util/HashtableSE.h"
#if 0
#include "runtime/midp/Class.h"
#include "runtime/midp/Object.h"
namespace util {
template<class K,class V> class HashtableSE {
public:
    explicit HashtableSE(int capacity);
    ~HashtableSE();
    V* get(K* key) const;
    bool containsKey(K* key) const;
    V* put(K* key,V* value);

    class HashtableClass : public midp::Class {
    public:
        HashtableClass();
        virtual ~HashtableClass();
        virtual midp::String* getName() const;
        virtual midp::Object* newInstance() const;
    };
};
}
#endif
