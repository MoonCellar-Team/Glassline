#include <new>
#include <unordered_map>

namespace m3g {
class ShaderBinding;
}

namespace {

using Table = std::unordered_map<unsigned, m3g::ShaderBinding*>;

Table*& table(void* self) { return *static_cast<Table**>(self); }

}

extern "C" {

void hash_table_ctor(void* self) __asm__("_ZN10hash_tableIjPN3m3g13ShaderBindingEEC1Ev");
void hash_table_ctor(void* self) { table(self) = new Table(); }

void hash_table_dtor(void* self) __asm__("_ZN10hash_tableIjPN3m3g13ShaderBindingEED1Ev");
void hash_table_dtor(void* self) {
    delete table(self);
    table(self) = nullptr;
}

unsigned hash_table_size(const void* self) __asm__("_ZNK10hash_tableIjPN3m3g13ShaderBindingEE4sizeEv");
unsigned hash_table_size(const void* self) {
    return static_cast<unsigned>(table(const_cast<void*>(self))->size());
}

m3g::ShaderBinding** hash_table_at(void* self, const unsigned* key)
    __asm__("_ZN10hash_tableIjPN3m3g13ShaderBindingEEixERKj");
m3g::ShaderBinding** hash_table_at(void* self, const unsigned* key) { return &(*table(self))[*key]; }

}
