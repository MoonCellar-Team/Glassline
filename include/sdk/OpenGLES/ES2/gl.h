#pragma once

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef char GLchar;

#define GL_TEXTURE_2D 0x0DE1
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR 0x2601
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE2 0x84C2
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

extern "C" {

void glDeleteProgram(GLuint program);
void glDeleteShader(GLuint shader);
void glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
int glGetUniformLocation(GLuint program, const GLchar* name);
void glLinkProgram(GLuint program);
void glUniform2fv(GLint location, GLsizei count, const GLfloat* v);
void glValidateProgram(GLuint program);

}
