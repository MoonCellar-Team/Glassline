#include <new>
#include <unordered_map>

namespace m3g {
class ShaderBinding;
}

namespace {

using Table = std::unordered_map<unsigned, m3g::ShaderBinding*>;

Table*& table(void* self) { return *static_cast<Table**>(self); }

}

#ifdef __wasm__
using Self = void*;
#define MEDECOMP_RETURN_SELF return self
#else
using Self = void;
#define MEDECOMP_RETURN_SELF return
#endif

extern "C" {

Self hash_table_ctor(void* self) MEDECOMP_SYMBOL("_ZN10hash_tableIjPN3m3g13ShaderBindingEEC1Ev");
Self hash_table_ctor(void* self) {
    table(self) = new Table();
    MEDECOMP_RETURN_SELF;
}

Self hash_table_dtor(void* self) MEDECOMP_SYMBOL("_ZN10hash_tableIjPN3m3g13ShaderBindingEED1Ev");
Self hash_table_dtor(void* self) {
    delete table(self);
    table(self) = nullptr;
    MEDECOMP_RETURN_SELF;
}

unsigned hash_table_size(const void* self) MEDECOMP_SYMBOL("_ZNK10hash_tableIjPN3m3g13ShaderBindingEE4sizeEv");
unsigned hash_table_size(const void* self) {
    return static_cast<unsigned>(table(const_cast<void*>(self))->size());
}

m3g::ShaderBinding** hash_table_at(void* self, const unsigned* key)
    MEDECOMP_SYMBOL("_ZN10hash_tableIjPN3m3g13ShaderBindingEEixERKj");
m3g::ShaderBinding** hash_table_at(void* self, const unsigned* key) { return &(*table(self))[*key]; }

}
