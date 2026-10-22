#pragma once
#include "m3g/ShaderCache.h"
#include "m3g/OpaqueObject.h"
namespace midp { class GLES20; }
namespace m3g {
class ShaderBinding;
namespace StockES2Shaders { enum StockES2Shaders : int; }
using ShaderTableStorage=OpaqueObject<hash_table<unsigned,ShaderBinding*>,24>;
}

namespace m3g {

class OpenGLES20ShaderSupport {
public:
    OpenGLES20ShaderSupport();
    static OpenGLES20ShaderSupport* getInstance();
    void notifyCacheInvalidation();
    void warmCache();
    static bool extractParameters(ShaderBinding* binding,unsigned flags);
    static void extractCustomShaderParameters(ShaderBinding* binding);
    static int generateCustomProgram(int vertex,int pixel);
    static void bindAttributes(int program,int flags);
    int generateProgram(int vertex,int pixel,unsigned flags);
    static void bindCustomShaderAttributes(ShaderBinding* binding);
    static bool compileShader(int unused,int shader);
    static int generatePixelShader(unsigned flags,const char* header,const char* beforeColour,const char* afterColour,const char* footer);
    static int generateVertexShader(unsigned flags,const char* header,const char* footer);
    ShaderBinding* createProgram(unsigned flags);
    void* getStockShader(StockES2Shaders::StockES2Shaders type);
    ShaderBinding* lookupProgram(unsigned flags);
    void* bindProgram(unsigned flags);
    ShaderTableStorage m_programs;
    unsigned m_currentFlags;
    ShaderBinding* m_current;
    midp::GLES20* m_gl;
    ShaderBinding* m_stock[6];
};

}
