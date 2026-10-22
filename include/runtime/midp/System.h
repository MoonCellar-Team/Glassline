#pragma once

namespace midp {

class String;

class System {
public:
    static long long currentTimeMillis();
    static String* getProperty(String* key);
    static void arraycopy(const float* src, int srcPos, float* dst, int dstPos, int length);
    static void arraycopy(const void* src, int srcPos, const void* dst, int dstPos, int length,
                          int elementSize);
    static void printf(const char* format, ...);
    static void println(const char* text);
};

}
