#include "m3g/OpenGLES20ShaderSupport.h"
#include "m3g/ShaderBinding.h"
#include "m3g/VertexHelpers.h"
#include "runtime/midp/GLES20.h"
#include "m3g/RendererSupport.h"
#include "runtime/midp/StringBuffer.h"
#include "sdk/vector.h"
#ifdef MEDECOMP_PORT
extern "C" const char* medecomp_texture_bias(void);
extern "C" int medecomp_sign_sharpen(void);
#endif
extern "C" {
void glGetIntegerv(unsigned,int*); int glGetUniformLocation(unsigned,const char*); unsigned glCreateProgram();
void glAttachShader(unsigned,unsigned); void glBindAttribLocation(unsigned,unsigned,const char*);
void glLinkProgram(unsigned); void glGetProgramiv(unsigned,unsigned,int*); void glGetProgramInfoLog(unsigned,int,int*,char*);
void glDeleteProgram(unsigned); void glValidateProgram(unsigned); void glDeleteShader(unsigned);
void glCompileShader(unsigned); void glGetShaderiv(unsigned,unsigned,int*); void glGetShaderInfoLog(unsigned,int,int*,char*); void glGetShaderSource(unsigned,int,int*,char*);
unsigned glCreateShader(unsigned); void glShaderSource(unsigned,int,const char* const*,const int*);
int snprintf(char*,__SIZE_TYPE__,const char*,...); int printf(const char*,...);
}
namespace midp { class DisplayIPhone { public: static DisplayIPhone* getStaticDisplay(); void* getGL(); }; }
namespace m3g {
static OpenGLES20ShaderSupport* g_OpenGLES20SSInstance=nullptr;
namespace {
struct ShaderText {
    OpaqueObject<midp::StringBuffer,16> storage;
    void append(const char* value) { storage.get()->append(value); }
    void append(unsigned value) { storage.get()->append(value); }
    void copy(std::vector<char>& output) { int length=storage.get()->length(); output.resize(length+1,'\0'); storage.get()->copyCString(&output[0],output.size()); }
};
}
}

namespace m3g {

OpenGLES20ShaderSupport::OpenGLES20ShaderSupport() : m_programs(),m_currentFlags(~0u),m_current(nullptr),m_gl(static_cast<midp::GLES20*>(midp::DisplayIPhone::getStaticDisplay()->getGL())) {
    g_OpenGLES20SSInstance=this; int attributes,vertexUniforms,fragmentUniforms,varyings;
    glGetIntegerv(0x8869,&attributes); glGetIntegerv(0x8dfb,&vertexUniforms); glGetIntegerv(0x8dfd,&fragmentUniforms); glGetIntegerv(0x8dfc,&varyings);
    notifyCacheInvalidation(); for(int i=0;i<6;++i) m_stock[i]=nullptr;
}

OpenGLES20ShaderSupport* OpenGLES20ShaderSupport::getInstance() { if(!g_OpenGLES20SSInstance) g_OpenGLES20SSInstance=new OpenGLES20ShaderSupport; return g_OpenGLES20SSInstance; }

void OpenGLES20ShaderSupport::notifyCacheInvalidation() { m_currentFlags=~0u; }

void OpenGLES20ShaderSupport::warmCache() {
    m_gl->glDisableVertexAttribArray(5); m_gl->glDisableVertexAttribArray(6); m_gl->glDisableVertexAttribArray(2); m_gl->glDisableVertexAttribArray(1); m_gl->glEnableVertexAttribArray(0);
    short vertices[9]={0}; glVertexAttribPointer(0,3,0x1402,0,0,vertices); m_gl->glEnableVertexAttribArray(0); glDrawArrays(4,0,1); m_gl->glDisableVertexAttribArray(0);
}

bool OpenGLES20ShaderSupport::extractParameters(ShaderBinding* binding,unsigned flags) {
    for(int i=0;i<6;++i) binding->location(i)=-1;
    ensureNoGLError(); binding->location(0)=glGetUniformLocation(binding->m_program,"g_WorldViewProjection"); bool valid=binding->location(0)!=-1;
    if(flags&1) { binding->location(1)=glGetUniformLocation(binding->m_program,"g_World"); if(binding->location(1)==-1) valid=false; }
    char name[1028];
    if(flags&4) for(int i=0;i<20;++i) { snprintf(name,1024,"g_SkinPalette[%u]",i); binding->palette()[i]=glGetUniformLocation(binding->m_program,name); if(binding->palette()[i]==-1) valid=false; }
    for(unsigned unit=0;unit<4;++unit) {
        unsigned blend=(flags>>(16+unit*3))&7; if(!blend || !(flags&(3u<<(4+unit*2)))) break;
        int* texture=binding->texture(unit); texture[0]=texture[1]=texture[2]=-1;
        snprintf(name,1024,"g_Texture%u",unit); texture[0]=glGetUniformLocation(binding->m_program,name);
        snprintf(name,1024,"g_TextureTransform%u",unit); texture[1]=glGetUniformLocation(binding->m_program,name);
        if(texture[0]==-1 || texture[1]==-1) valid=false;
        if(blend==2) { snprintf(name,1024,"g_TextureEnvColour%u",unit); texture[2]=glGetUniformLocation(binding->m_program,name); if(texture[2]==-1) valid=false; }
    }
    if(flags&0x2000) { binding->location(3)=glGetUniformLocation(binding->m_program,"g_AlphaThreshhold"); if(binding->location(3)==-1) valid=false; }
    if(flags&0xc000) { binding->location(4)=glGetUniformLocation(binding->m_program,"g_FogColour"); if(binding->location(4)==-1) valid=false; binding->location(5)=glGetUniformLocation(binding->m_program,(flags&0xc000)==0x4000 ? "g_Fog" : "g_FogDensity"); if(binding->location(5)==-1) valid=false; }
    if(flags&0x1000) { binding->location(2)=glGetUniformLocation(binding->m_program,"g_UniformColour"); if(binding->location(2)==-1) valid=false; }
    ensureNoGLError(); return valid;
}

void OpenGLES20ShaderSupport::extractCustomShaderParameters(ShaderBinding* binding) { extractParameters(binding,binding->m_flags); }

int OpenGLES20ShaderSupport::generateCustomProgram(int vertex,int pixel) { int program=glCreateProgram(); glAttachShader(program,vertex); glAttachShader(program,pixel); return program; }

void OpenGLES20ShaderSupport::bindAttributes(int program,int flags) {
    glBindAttribLocation(program,0,"vs_Position"); if(flags&1) glBindAttribLocation(program,1,"vs_Normals"); if(flags&2) glBindAttribLocation(program,2,"vs_Colour");
    if(flags&4) { glBindAttribLocation(program,3,"vs_SkinIndices"); glBindAttribLocation(program,4,"vs_SkinWeights"); }
    char name[260]; for(unsigned unit=0;unit<4 && ((static_cast<unsigned>(flags)>>(4+unit*2))&3);++unit) { snprintf(name,256,"vs_TexCoord%u",unit); glBindAttribLocation(program,unit+5,name); }
}

int OpenGLES20ShaderSupport::generateProgram(int vertex,int pixel,unsigned flags) {
    int program=glCreateProgram(); ensureNoGLError(); glAttachShader(program,vertex); glAttachShader(program,pixel); ensureNoGLError(); bindAttributes(program,flags); ensureNoGLError(); glLinkProgram(program); ensureNoGLError();
    int status; glGetProgramiv(program,0x8b82,&status);
    if(status==0) { int length=0; glGetProgramiv(program,0x8b84,&length); if(length>1) { char* log=new char[length]; glGetProgramInfoLog(program,length,nullptr,log); printf("Shader linkage error!\n%s",log); delete[] log; glDeleteProgram(program); return 0; } }
    ensureNoGLError(); glValidateProgram(program); ensureNoGLError(); glDeleteShader(vertex); glDeleteShader(pixel); ensureNoGLError(); return program;
}

void OpenGLES20ShaderSupport::bindCustomShaderAttributes(ShaderBinding* binding) { bindAttributes(binding->m_program,binding->m_flags); }

bool OpenGLES20ShaderSupport::compileShader(int,int shader) {
    ensureNoGLError(); glCompileShader(shader); ensureNoGLError(); int status; glGetShaderiv(shader,0x8b81,&status);
    if(status==0) { int length=0; glGetShaderiv(shader,0x8b84,&length); if(length>1) {
        char* log=new char[length]; glGetShaderInfoLog(shader,length,nullptr,log); printf("Shader compilation error!\n%s",log); delete[] log;
        glGetShaderiv(shader,0x8b88,&length); char* source=new char[length]; glGetShaderSource(shader,length,nullptr,source); printf("SOURCE ==>\n%s",source); delete[] source; glDeleteShader(shader); return false;
    } }
    ensureNoGLError(); return true;
}

ShaderBinding* OpenGLES20ShaderSupport::createProgram(unsigned flags) {
#ifdef MEDECOMP_PORT
    bool shadow=(flags&MEDECOMP_SHADOW_FLAG) && !(flags&8);
    int vertex=generateVertexShader(flags,shadow ? medecomp_shadow_vertex_header : nullptr,shadow ? medecomp_shadow_vertex_footer : nullptr);
    int pixel=generatePixelShader(flags,shadow ? medecomp_shadow_pixel_header : nullptr,nullptr,shadow ? medecomp_shadow_pixel_after : nullptr,nullptr); int program=generateProgram(vertex,pixel,flags);
#else
    int vertex=generateVertexShader(flags,nullptr,nullptr); int pixel=generatePixelShader(flags,nullptr,nullptr,nullptr,nullptr); int program=generateProgram(vertex,pixel,flags);
#endif
    ShaderBinding* binding=new ShaderBinding(flags); binding->m_program=program; extractParameters(binding,flags); m_gl->glUseProgram(binding->m_program); warmCache(); return binding;
}

ShaderBinding* OpenGLES20ShaderSupport::lookupProgram(unsigned flags) {
    unsigned before=m_programs.get()->size(); ShaderBinding*& binding=(*m_programs.get())[flags]; unsigned after=m_programs.get()->size(); if(before<after) binding=createProgram(flags); return binding;
}

void* OpenGLES20ShaderSupport::bindProgram(unsigned flags) {
    if(m_currentFlags!=flags) { ShaderBinding* binding=lookupProgram(flags); m_currentFlags=flags; m_current=binding; }
    m_gl->glUseProgram(m_current->m_program); return m_current;
}

int OpenGLES20ShaderSupport::generateVertexShader(unsigned flags,const char* header,const char* footer) {
    const char* parts[24]; int count=0;
    parts[count++]=(flags&8) ? "uniform highp mat4 g_WorldViewProjection;\nattribute highp vec2 vs_Position;\n" : "uniform highp mat4 g_WorldViewProjection;\nattribute highp vec3 vs_Position;\n";
    if(flags&1) parts[count++]="uniform highp mat4 g_World;\nattribute mediump vec3 vs_Normals;\nvarying mediump vec3 v_Normals;\n";
    if(flags&2) parts[count++]="attribute mediump vec4 vs_Colour;\nvarying mediump vec4 v_Colour;\n";
    std::vector<char> skin, declarations, body;
    if(flags&4) { ShaderText text; text.append("attribute highp int4 vs_SkinIndices;\n"); text.append("attribute mediump float4 vs_SkinWeights;\n"); text.append("uniform highp mat4 g_SkinPalette["); text.append(20u); text.append("];\n"); text.copy(skin); parts[count++]=&skin[0]; }
    {
        ShaderText decl,main;
        for(unsigned unit=0;unit<4;++unit) {
            unsigned components=(flags>>(4+unit*2))&3; if(!components) break;
            decl.append(components==2 ? "attribute highp vec3 vs_TexCoord" : "attribute highp vec2 vs_TexCoord"); decl.append(unit); decl.append(";\n");
            decl.append("uniform highp mat4 g_TextureTransform"); decl.append(unit); decl.append(";\n"); decl.append("varying highp vec2 v_TexCoord"); decl.append(unit); decl.append(";\n");
            main.append("\tv_TexCoord"); main.append(unit); main.append(" = (g_TextureTransform"); main.append(unit); main.append(" * vec4(vs_TexCoord"); main.append(unit); main.append(components==2 ? ", 1)).xy;\n" : ", 0, 1)).xy;\n");
        }
        decl.copy(declarations); main.copy(body);
    }
    if(declarations.size()) parts[count++]=&declarations[0];
    if(flags&0xc000) parts[count++]="varying highp float v_Z;\n";
    if(header) parts[count++]=header;
    parts[count++]=(flags&4) ? "void main()\n{\n\thighp mat4 skintransform = vs_SkinWeights.x * g_SkinPalette[vs_SkinIndices.x] + vs_SkinWeights.z * g_SkinPalette[vs_SkinIndices.y] + vs_SkinWeights.y * g_SkinPalette[vs_SkinIndices.z] + vs_SkinWeights.w * g_SkinPalette[vs_SkinIndices.w];\n\thighp mat4 transform = g_WorldViewProjection * transform;\n" : "void main()\n{\n\thighp mat4 transform = g_WorldViewProjection;\n";
    parts[count++]=(flags&8) ? "\tgl_Position = transform * vec4(vs_Position, 0.0, 1.0);\n" : "\tgl_Position = transform * vec4(vs_Position, 1.0);\n";
    if(flags&1) parts[count++]=(flags&4) ? "\tv_Normals = normalize(mat3(g_World * skintransform) * vs_Normals);\n" : "\tv_Normals = normalize(mat3(g_World) * vs_Normals);\n";
    if(flags&2) parts[count++]="\tv_Colour = vs_Colour;\n";
    if(body.size()) parts[count++]=&body[0];
    if(flags&0xc000) parts[count++]="\tv_Z = gl_Position.w;\n";
    if(footer) parts[count++]=footer;
    parts[count++]="}\n";
    int shader=glCreateShader(0x8b31); ensureNoGLError(); glShaderSource(shader,count,parts,nullptr); compileShader(0,shader);
    int length=0; glGetShaderiv(shader,0x8b88,&length); char* source=new char[length]; glGetShaderSource(shader,length,nullptr,source); delete[] source; return shader;
}

int OpenGLES20ShaderSupport::generatePixelShader(unsigned flags,const char* header,const char* beforeColour,const char* afterColour,const char* footer) {
    const char* parts[28]; int count=0;
#ifdef MEDECOMP_PORT
    bool sharpen=medecomp_sign_sharpen();
    parts[count++]=sharpen || medecomp_shadows_enabled() ? "#extension GL_OES_standard_derivatives : enable\nprecision highp float;\n" : "precision highp float;\n";
    if(sharpen) parts[count++]="vec4 sampleTex(sampler2D s, vec2 uv, float bias) {\n\tvec4 c = texture2D(s, uv, bias);\n\tvec2 dx = dFdx(uv), dy = dFdy(uv);\n\tfloat span = max(max(abs(dx.x), abs(dx.y)), max(abs(dy.x), abs(dy.y)));\n\tfloat sharp = 1.0 - smoothstep(0.004, 0.012, span);\n\tif (sharp <= 0.0) return c;\n\tvec2 px = max(abs(dx), abs(dy));\n\tvec3 blur = (texture2D(s, uv + vec2(px.x, 0.0), bias).rgb + texture2D(s, uv - vec2(px.x, 0.0), bias).rgb + texture2D(s, uv + vec2(0.0, px.y), bias).rgb + texture2D(s, uv - vec2(0.0, px.y), bias).rgb) * 0.25;\n\treturn vec4(clamp(c.rgb + (c.rgb - blur) * sharp * 0.75, 0.0, 1.0), c.a);\n}\n";
#else
    parts[count++]="precision highp float;\n";
#endif
    if(flags&0x2000) parts[count++]="uniform lowp float g_AlphaThreshhold;\n";
    unsigned fog=flags&0xc000;
    if(fog==0xc000) parts[count++]="uniform mediump float g_FogDensity;\nvarying float v_Z;\nuniform mediump vec3 g_FogColour;\nvec4 Fog(in vec4 colour)\n{\n\tfloat fdensity = -g_FogDensity * v_Z;\n\tfloat fog = clamp(exp(fdensity * fdensity), 0.0, 1.0);\n\treturn vec4(((1.0 - fog) * g_FogColour + fog * colour.rgb), colour.a);\n}\n"; else if(fog==0x8000) parts[count++]="uniform mediump float g_FogDensity;\nvarying float v_Z;\nuniform mediump vec3 g_FogColour;\nvec4 Fog(in vec4 colour)\n{\n\tfloat fog = clamp(exp(-g_FogDensity * v_Z), 0.0, 1.0);\n\treturn vec4(((1.0 - fog) * g_FogColour + fog * colour.rgb), colour.a);\n}\n"; else if(fog==0x4000) parts[count++]="uniform highp vec2 g_Fog;\nvarying float v_Z;\nuniform mediump vec3 g_FogColour;\nvec4 Fog(in vec4 colour)\n{\n\tfloat fog = clamp((g_Fog.y - v_Z) / (g_Fog.y - g_Fog.x), 0.0, 1.0);\n\treturn vec4(((1.0 - fog) * g_FogColour + fog * colour.rgb), colour.a);\n}\n";
    if(flags&1) parts[count++]="varying highp vec3 v_Normals;\n";
    std::vector<char> declarations,body;
    {
        ShaderText decl,main;
        for(unsigned unit=0;unit<4;++unit) {
            unsigned mode=(flags>>(16+unit*3))&7; if(!mode) break;
            decl.append("uniform lowp sampler2D g_Texture"); decl.append(unit); decl.append(";\n"); decl.append("varying highp vec2 v_TexCoord"); decl.append(unit); decl.append(";\n");
            main.append("\tmediump vec4 tcol"); main.append(unit); main.append(" = ");
#ifdef MEDECOMP_PORT
            if(sharpen) { main.append("sampleTex(g_Texture"); main.append(unit); main.append(", v_TexCoord"); main.append(unit); main.append(", "); main.append(medecomp_texture_bias()); main.append(");\n"); }
            else { main.append("texture2D(g_Texture"); main.append(unit); main.append(", v_TexCoord"); main.append(unit); main.append(", "); main.append(medecomp_texture_bias()); main.append(");\n"); }
#else
            main.append("texture2D(g_Texture"); main.append(unit); main.append(", v_TexCoord"); main.append(unit); main.append(", -2.0);\n");
#endif
            switch(mode) {
            case 1: main.append("\tcolour = vec4((colour.rgb + tcol"); main.append(unit); main.append(".rgb), colour.a * tcol"); main.append(unit); main.append(".a);\n"); break;
            case 2:
                decl.append("uniform mediump vec3 g_TextureEnvColour"); decl.append(unit); decl.append(";\n");
                main.append("\tcolour = vec4((colour.rgb * (vec3(1.0, 1.0, 1.0) - tcol"); main.append(unit); main.append(".rgb) + g_TextureEnvColour"); main.append(unit); main.append(" * tcol"); main.append(unit); main.append(".rgb), colour.a * tcol"); main.append(unit); main.append(".a);\n"); break;
            case 3: main.append("\tcolour = vec4((colour.rgb * (1.0 - tcol"); main.append(unit); main.append(".a) + tcol"); main.append(unit); main.append(".a * tcol"); main.append(unit); main.append(".rgb), colour.a);\n"); break;
            case 4: main.append("\tcolour *= tcol"); main.append(unit); main.append(";\n"); break;
            default: main.append("\tcolour = tcol"); main.append(unit); main.append(";\n");
            }
        }
        decl.copy(declarations); main.copy(body);
    }
    if(declarations.size()) parts[count++]=&declarations[0];
    if(flags&2) parts[count++]="varying mediump vec4 v_Colour;\n";
    if(header) parts[count++]=header;
    parts[count++]=(flags&0x1000) ? "uniform mediump vec4 g_UniformColour;\nvoid main()\n{\n\tmediump vec4 colour = vec4(1.0, 1.0, 1.0, 1.0);\n" : "void main()\n{\n\tmediump vec4 colour = vec4(1.0, 1.0, 1.0, 1.0);\n";
    if(beforeColour) parts[count++]=beforeColour;
    if(flags&2) parts[count++]="\tcolour *= v_Colour;\n";
    if(flags&0x1000) parts[count++]="\tcolour *= g_UniformColour;\n";
    if(body.size()) parts[count++]=&body[0];
    if(afterColour) parts[count++]=afterColour;
    if(fog) parts[count++]="\tcolour = Fog(colour);\n";
    if(flags&0x2000) parts[count++]="\tif (colour.a < g_AlphaThreshhold)\n\t\tdiscard;\n";
    if(footer) parts[count++]=footer;
    parts[count++]="\tgl_FragColor = colour;\n}\n";
    int shader=glCreateShader(0x8b30); ensureNoGLError(); glShaderSource(shader,count,parts,nullptr); compileShader(0,shader);
    int length=0; glGetShaderiv(shader,0x8b88,&length); char* source=new char[length]; glGetShaderSource(shader,length,nullptr,source); delete[] source; return shader;
}

void* OpenGLES20ShaderSupport::getStockShader(StockES2Shaders::StockES2Shaders type) {
    ShaderBinding*& binding=m_stock[static_cast<int>(type)];
    if(!binding) switch(static_cast<int>(type)) {
    case 0: binding=createProgram(0x1000); break; case 1: binding=createProgram(0x41010); break;
    case 2: {
        binding=new ShaderBinding(0x51010); int vertex=generateVertexShader(binding->m_flags,nullptr,nullptr);
        const char* source="precision highp float;\nuniform mediump vec4 g_UniformColour;\nuniform lowp sampler2D g_Texture0;\nvarying highp vec2 v_TexCoord0;\nvoid main()\n{\n\tmediump vec4 colour = vec4(1.0, 1.0, 1.0, 1.0);\n\tmediump vec4 tcol0 = texture2D(g_Texture0, v_TexCoord0);\n\tcolour = vec4(tcol0.rgb - g_UniformColour.rgb, tcol0.a);\n\tgl_FragColor = colour;\n}\n"; int pixel=glCreateShader(0x8b30); glShaderSource(pixel,1,&source,nullptr); compileShader(0,pixel);
        binding->m_program=generateProgram(vertex,pixel,binding->m_flags); extractParameters(binding,binding->m_flags); break;
    }
    case 3: binding=createProgram(0x50010); break; case 4: binding=createProgram(0x1008); break; case 5: binding=createProgram(0x50018); break;
    }
    m_currentFlags=~0u; return binding;
}

}
