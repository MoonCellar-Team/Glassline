#include "game/ReflectionMapShader.h"

#include <OpenGLES/ES2/gl.h>

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "m3g/GLHelpers.h"
#include "m3g/Image2D.h"
#include "m3g/OpenGLES20ShaderSupport.h"
#include "m3g/Transform.h"
#include "runtime/midp/GLES20.h"

#ifdef MEDECOMP_PORT
#include <string>
#endif

static const int IMAGE_REFLECTION_DEFAULT = 129;

static const int SHADER_FLAGS = 0x51011;

static const int REFLECTION_TEXTURE_UNIT = 2;

void ReflectionMapShader::setReflectionParameters(float scale, float intensity) {
    m_parameters[0] = scale;
    m_parameters[1] = intensity;
}

void ReflectionMapShader::setCameraPos(float x, float y, float z) {
    m_cameraPos[0] = x;
    m_cameraPos[1] = y;
    m_cameraPos[2] = z;
}

void ReflectionMapShader::setReflectionMap(m3g::Image2D* image) {
    midp::INCREF(image);
    midp::DECREF(m_reflectionMap);
    m_reflectionMap = image;
}

void ReflectionMapShader::setView(const m3g::Transform& view) {
    float matrix[16];
    view.get(matrix);
    transpose(matrix);
    gl()->glUniformMatrix4fv(m_viewLocation, 1, 0, matrix);
}

void ReflectionMapShader::setScaledWorld(const m3g::Transform& world) {
    float matrix[16];
    world.get(matrix);
    transpose(matrix);
    gl()->glUniformMatrix4fv(m_worldScaledLocation, 1, 0, matrix);
}

m3g::ShaderBinding* ReflectionMapShader::bind(const m3g::Transform& transform) {
    gl()->glUseProgram(m_binding.m_program);
    float matrix[16];
    transform.get(matrix);
    transpose(matrix);
    gl()->glUniform3fv(m_cameraPosLocation, 1, m_cameraPos);
    gl()->glActiveTexture(GL_TEXTURE2);
    m3g::Image2DBindGLES20(gl(), m_reflectionMap, GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
                           GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    gl()->glUniform1i(m_samplerLocation, REFLECTION_TEXTURE_UNIT);
    glUniform2fv(m_parametersLocation, 1, m_parameters);
    return &m_binding;
}

ReflectionMapShader::~ReflectionMapShader() {
    midp::DECREF(m_reflectionMap);
    glDeleteProgram(m_binding.m_program);
}

int ReflectionMapShader::generatePShader(int flags) {
    const char* header =
        "uniform lowp sampler2D g_ReflectionSampler;\n"
        "varying highp vec3 v_PixelToCamera;\n"
        "uniform highp vec2 g_ReflectionShaderParameters;\n";
    const char* afterColour =
        "\tmediump vec3 refcoord = vec3(0.5, 0.5, 0.5) + reflect(v_PixelToCamera / abs(v_PixelToCamera.z), "
        "v_Normals) * g_ReflectionShaderParameters.x;\n"
        "\tlowp vec3 refcolour = texture2D(g_ReflectionSampler, vec2(refcoord.x, 1.0 - refcoord.y)).rgb;\n"
        "\tcolour = vec4(colour.rgb + refcolour * (1.0 - colour.a) * g_ReflectionShaderParameters.y, 1.0);\n";
#ifdef MEDECOMP_PORT
    std::string shadowHeader = std::string(header) + medecomp_shadow_pixel_header;
    std::string shadowAfter = std::string(afterColour) + medecomp_shadow_pixel_after;
    if (medecomp_shadows_enabled()) {
        header = shadowHeader.c_str();
        afterColour = shadowAfter.c_str();
    }
#endif
    return m3g::OpenGLES20ShaderSupport::generatePixelShader(flags, header, NULL, afterColour, NULL);
}

int ReflectionMapShader::generateVShader(int flags) {
    const char* header =
        "uniform highp mat4 g_WorldScaled;\n"
        "uniform highp mat4 g_View;\n"
        "uniform highp vec3 g_CameraPos;\n"
        "varying highp vec3 v_PixelToCamera;\n";
    const char* footer =
        "\tv_PixelToCamera = (g_View * vec4(((g_WorldScaled * vec4(vs_Position, 1.0)).xyz - g_CameraPos), "
        "1.0)).xyz;\n";
#ifdef MEDECOMP_PORT
    std::string shadowHeader = std::string(header) + medecomp_shadow_vertex_header;
    std::string shadowFooter = std::string(footer) + medecomp_shadow_vertex_footer;
    if (medecomp_shadows_enabled()) {
        header = shadowHeader.c_str();
        footer = shadowFooter.c_str();
    }
#endif
    return m3g::OpenGLES20ShaderSupport::generateVertexShader(flags, header, footer);
}

ReflectionMapShader::ReflectionMapShader()
    : m_binding(SHADER_FLAGS),
      m_samplerLocation(0),
      m_worldScaledLocation(0),
      m_viewLocation(0),
      m_cameraPosLocation(0),
      m_parametersLocation(0),
      m_reflectionMap(NULL) {
    int flags = m_binding.m_flags;
    int vertexShader = generateVShader(flags);
    int pixelShader = generatePShader(flags);
    int program = m3g::OpenGLES20ShaderSupport::generateCustomProgram(vertexShader, pixelShader);
    m_binding.m_program = program;
    m3g::OpenGLES20ShaderSupport::bindCustomShaderAttributes(&m_binding);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == 0) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1) {
            char* log = new char[logLength];
            glGetProgramInfoLog(program, logLength, NULL, log);
            delete[] log;
            glDeleteProgram(program);
        }
    }
    glValidateProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(pixelShader);

    m_samplerLocation = glGetUniformLocation(m_binding.m_program, "g_ReflectionSampler");
    m_viewLocation = glGetUniformLocation(m_binding.m_program, "g_View");
    m_worldScaledLocation = glGetUniformLocation(m_binding.m_program, "g_WorldScaled");
    m_cameraPosLocation = glGetUniformLocation(m_binding.m_program, "g_CameraPos");
    m_parametersLocation = glGetUniformLocation(m_binding.m_program, "g_ReflectionShaderParameters");
    m3g::OpenGLES20ShaderSupport::extractCustomShaderParameters(&m_binding);

    setReflectionParameters(0.4f, 1.0f);
    m3g::Image2D* image = AppEngine::getCanvas()->getResourceManager().loadM3GImage2D(IMAGE_REFLECTION_DEFAULT);
    setReflectionMap(image);
    midp::DECREF(image);
}
