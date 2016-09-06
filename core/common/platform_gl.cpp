#include "gl.h"
#include "gl/error.h"

// #if PLATFORM_LINUX
// #define GL_GLEXT_PROTOTYPES
// #include <GLFW/glfw3.h>
// #endif

// #if defined(PLATFORM_ANDROID) || defined(PLATFORM_RPI)
// typedef long GLsizeiptr;
// typedef long GLintptr;
// #else
// #include <stddef.h>
// // typedef ptrdiff_t GLsizeiptr;
// // typedef ptrdiff_t GLintptr;
// #endif

#ifdef PLATFORM_OSX
    #define glClearDepthf glClearDepth
    #define glDepthRangef glDepthRange
    #define glDeleteVertexArrays glDeleteVertexArraysAPPLE
    #define glGenVertexArrays glGenVertexArraysAPPLE
    #define glBindVertexArray glBindVertexArrayAPPLE
#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS) || defined(PLATFORM_RPI)
    #define glMapBuffer glMapBufferOES
    #define glUnmapBuffer glUnmapBufferOES
#endif

#if defined(PLATFORM_IOS)
    #define glDeleteVertexArrays glDeleteVertexArraysOES
    #define glGenVertexArrays glGenVertexArraysOES
    #define glBindVertexArray glBindVertexArrayOES
#endif

#ifdef PLATFORM_ANDROID
#define GL_APICALL  __attribute__((visibility("default")))
#define GL_APIENTRY
#define GL_APIENTRYP GL_APIENTRY*
#else
#define GL_APICALL
#define GL_APIENTRY
#endif

extern "C" {

    GL_APICALL GLenum GL_APIENTRY glGetError(void);
    GL_APICALL const GLubyte* GL_APIENTRY glGetString(GLenum name);

    GL_APICALL void GL_APIENTRY glClear(GLbitfield mask);
    GL_APICALL void GL_APIENTRY glLineWidth(GLfloat width);
    GL_APICALL void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

    GL_APICALL void GL_APIENTRY glEnable(GLenum);
    GL_APICALL void GL_APIENTRY glDisable(GLenum);
    GL_APICALL void GL_APIENTRY glDepthFunc(GLenum func);
    GL_APICALL void GL_APIENTRY glDepthMask(GLboolean flag);
#ifdef PLATFORM_OSX
    GL_APICALL void GL_APIENTRY glDepthRange(GLclampd n, GLclampd f);
    GL_APICALL void GL_APIENTRY glClearDepth(GLclampd d);
#else
    GL_APICALL void GL_APIENTRY glDepthRangef(GLfloat n, GLfloat f);
    GL_APICALL void GL_APIENTRY glClearDepthf(GLfloat d);
#endif
    GL_APICALL void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor);
    GL_APICALL void GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask);
    GL_APICALL void GL_APIENTRY glStencilMask(GLuint mask);
    GL_APICALL void GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
    GL_APICALL void GL_APIENTRY glClearStencil(GLint s);
    GL_APICALL void GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    GL_APICALL void GL_APIENTRY glCullFace(GLenum mode);
    GL_APICALL void GL_APIENTRY glFrontFace(GLenum mode);
    GL_APICALL void GL_APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    GL_APICALL void GL_APIENTRY glGetIntegerv(GLenum pname, GLint *params );

    // Program
    GL_APICALL void GL_APIENTRY glUseProgram(GLuint program);
    GL_APICALL void GL_APIENTRY glDeleteProgram(GLuint program);
    GL_APICALL void GL_APIENTRY glDeleteShader(GLuint shader);
    GL_APICALL GLuint GL_APIENTRY glCreateShader(GLenum type);
    GL_APICALL GLuint GL_APIENTRY glCreateProgram();
    GL_APICALL void GL_APIENTRY glCompileShader(GLuint shader);
    GL_APICALL void GL_APIENTRY glAttachShader(GLuint program, GLuint shader);
    GL_APICALL void GL_APIENTRY glLinkProgram(GLuint program);
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_RPI)
    GL_APICALL void GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint *length);
#else
    GL_APICALL void GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
#endif
    GL_APICALL void GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    GL_APICALL void GL_APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    GL_APICALL GLint GL_APIENTRY glGetUniformLocation(GLuint program, const GLchar *name);
    GL_APICALL GLint GL_APIENTRY glGetAttribLocation(GLuint program, const GLchar *name);
    GL_APICALL void GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint *params);
    GL_APICALL void GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint *params);

    // Buffers
    GL_APICALL void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer);
    GL_APICALL void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint *buffers);
    GL_APICALL void GL_APIENTRY glGenBuffers(GLsizei n, GLuint *buffers);
    GL_APICALL void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
    GL_APICALL void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);

    // Texture
    GL_APICALL void GL_APIENTRY glBindTexture(GLenum target, GLuint texture );
    GL_APICALL void GL_APIENTRY glActiveTexture(GLenum texture);
    GL_APICALL void GL_APIENTRY glGenTextures(GLsizei n, GLuint *textures );
    GL_APICALL void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures);
    GL_APICALL void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param );
    GL_APICALL void GL_APIENTRY glTexImage2D(GLenum target, GLint level,
                       GLint internalFormat,
                       GLsizei width, GLsizei height,
                       GLint border, GLenum format, GLenum type,
                       const GLvoid *pixels);

    GL_APICALL void GL_APIENTRY glTexSubImage2D(GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLsizei width, GLsizei height,
                          GLenum format, GLenum type,
                          const GLvoid *pixels);

    GL_APICALL void GL_APIENTRY glGenerateMipmap(GLenum target);


    GL_APICALL void GL_APIENTRY glEnableVertexAttribArray(GLuint index);
    GL_APICALL void GL_APIENTRY glDisableVertexAttribArray(GLuint index);
    GL_APICALL void GL_APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                                GLsizei stride, const void *pointer);

    GL_APICALL void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count );
    GL_APICALL void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count,
                         GLenum type, const GLvoid *indices );

    GL_APICALL void GL_APIENTRY glUniform1f(GLint location, GLfloat v0);
    GL_APICALL void GL_APIENTRY glUniform2f(GLint location, GLfloat v0, GLfloat v1);
    GL_APICALL void GL_APIENTRY glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    GL_APICALL void GL_APIENTRY glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

    GL_APICALL void GL_APIENTRY glUniform1i(GLint location, GLint v0);
    GL_APICALL void GL_APIENTRY glUniform2i(GLint location, GLint v0, GLint v1);
    GL_APICALL void GL_APIENTRY glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
    GL_APICALL void GL_APIENTRY glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

    GL_APICALL void GL_APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat *value);
    GL_APICALL void GL_APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
    GL_APICALL void GL_APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
    GL_APICALL void GL_APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
    GL_APICALL void GL_APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint *value);
    GL_APICALL void GL_APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint *value);
    GL_APICALL void GL_APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint *value);
    GL_APICALL void GL_APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint *value);

    GL_APICALL void GL_APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    GL_APICALL void GL_APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    GL_APICALL void GL_APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

    // mapbuffer
    GL_APICALL void *GL_APIENTRY glMapBuffer(GLenum target, GLenum access);
    GL_APICALL GLboolean GL_APIENTRY glUnmapBuffer(GLenum target);

    GL_APICALL void GL_APIENTRY glFinish(void);

    GL_APICALL void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, GLvoid* pixels);

    // VAO
#ifdef PLATFORM_ANDROID
    typedef void (GL_APIENTRYP PFNGLBINDVERTEXARRAYOESPROC) (GLuint array);
    typedef void (GL_APIENTRYP PFNGLDELETEVERTEXARRAYSOESPROC) (GLsizei n, const GLuint *arrays);
    typedef void (GL_APIENTRYP PFNGLGENVERTEXARRAYSOESPROC) (GLsizei n, GLuint *arrays);
    typedef GLboolean (GL_APIENTRYP PFNGLISVERTEXARRAYOESPROC) (GLuint array);

    // defined in platform_android.cpp
    extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT;
    extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT;
    extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT;

    #define glDeleteVertexArrays glDeleteVertexArraysOESEXT
    #define glGenVertexArrays glGenVertexArraysOESEXT
    #define glBindVertexArray glBindVertexArrayOESEXT

#else
    GL_APICALL void GL_APIENTRY glBindVertexArray(GLuint array);
    GL_APICALL void GL_APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
    GL_APICALL void GL_APIENTRY glGenVertexArrays(GLsizei n, GLuint *arrays);
#endif

};



namespace Tangram {

GLenum GL::getError() {
    auto result = glGetError();
    GL_CHECK();
    return result;
}

const GLubyte* GL::getString(GLenum name) {
    auto result = glGetString(name);
    GL_CHECK();
    return result;
}

void GL::clear(GLbitfield mask) {
    GL_CHECK(glClear(mask));
}
void GL::lineWidth(GLfloat width) {
    GL_CHECK(glLineWidth(width));
}
void GL::viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    GL_CHECK(glViewport(x, y, width, height));
}

void GL::enable(GLenum id) {
    GL_CHECK(glEnable(id));
}
void GL::disable(GLenum id) {
    GL_CHECK(glDisable(id));
}
void GL::depthFunc(GLenum func) {
    GL_CHECK(glDepthFunc(func));
}
void GL::depthMask(GLboolean flag) {
    GL_CHECK(glDepthMask(flag));
}
void GL::depthRange(GLfloat n, GLfloat f) {
    GL_CHECK(glDepthRangef(n, f));
}
void GL::clearDepth(GLfloat d) {
    GL_CHECK(glClearDepthf(d));
}
void GL::blendFunc(GLenum sfactor, GLenum dfactor) {
    GL_CHECK(glBlendFunc(sfactor, dfactor));
}
void GL::stencilFunc(GLenum func, GLint ref, GLuint mask) {
    GL_CHECK(glStencilFunc(func, ref, mask));
}
void GL::stencilMask(GLuint mask) {
    GL_CHECK(glStencilMask(mask));
}
void GL::stencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    GL_CHECK(glStencilOp(fail, zfail, zpass));
}
void GL::clearStencil(GLint s) {
    GL_CHECK(glClearStencil(s));
}
void GL::colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GL_CHECK(glColorMask(red, green, blue, alpha));
}
void GL::cullFace(GLenum mode) {
    GL_CHECK(glCullFace(mode));
}
void GL::frontFace(GLenum mode) {
    GL_CHECK(glFrontFace(mode));
}
void GL::clearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    GL_CHECK(glClearColor(red, green, blue, alpha));
}
void GL::getIntegerv(GLenum pname, GLint *params ) {
    GL_CHECK(glGetIntegerv(pname, params ));
}

// Program
void GL::useProgram(GLuint program) {
    GL_CHECK(glUseProgram(program));
}
void GL::deleteProgram(GLuint program) {
    GL_CHECK(glDeleteProgram(program));
}
void GL::deleteShader(GLuint shader) {
    GL_CHECK(glDeleteShader(shader));
}
GLuint GL::createShader(GLenum type) {
    auto result = glCreateShader(type);
    GL_CHECK();
    return result;
}
GLuint GL::createProgram() {
    auto result = glCreateProgram();
    GL_CHECK();
    return result;
}

void GL::compileShader(GLuint shader) {
    GL_CHECK(glCompileShader(shader));
}
void GL::attachShader(GLuint program, GLuint shader) {
    GL_CHECK(glAttachShader(program,shader));
}
void GL::linkProgram(GLuint program) {
    GL_CHECK(glLinkProgram(program));
}

void GL::shaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
        auto source = const_cast<const GLchar**>(string);
    GL_CHECK(glShaderSource(shader, count, source, length));
}
void GL::getShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    GL_CHECK(glGetShaderInfoLog(shader, bufSize, length, infoLog));
}
void GL::getProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    GL_CHECK(glGetProgramInfoLog(program, bufSize, length, infoLog));
}
GLint GL::getUniformLocation(GLuint program, const GLchar *name) {
    return GL_CHECK(glGetUniformLocation(program, name));
}
GLint GL::getAttribLocation(GLuint program, const GLchar *name) {
    return GL_CHECK(glGetAttribLocation(program, name));
}
void GL::getProgramiv(GLuint program, GLenum pname, GLint *params) {
    GL_CHECK(glGetProgramiv(program,pname,params));
}
void GL::getShaderiv(GLuint shader, GLenum pname, GLint *params) {
    GL_CHECK(glGetShaderiv(shader,pname, params));
}

// Buffers
void GL::bindBuffer(GLenum target, GLuint buffer) {
    GL_CHECK(glBindBuffer(target, buffer));
}
void GL::deleteBuffers(GLsizei n, const GLuint *buffers) {
    GL_CHECK(glDeleteBuffers(n, buffers));
}
void GL::genBuffers(GLsizei n, GLuint *buffers) {
    GL_CHECK(glGenBuffers(n, buffers));
}
void GL::bufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    GL_CHECK(glBufferData(target, size, data, usage));
}
void GL::bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    GL_CHECK(glBufferSubData(target, offset, size, data));
}
void GL::readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, GLvoid* pixels) {
    GL_CHECK(glReadPixels(x, y, width, height, format, type, pixels));
}

// Texture
void GL::bindTexture(GLenum target, GLuint texture ) {
    GL_CHECK(glBindTexture(target, texture ));
}
void GL::activeTexture(GLenum texture) {
    GL_CHECK(glActiveTexture(texture));
}
void GL::genTextures(GLsizei n, GLuint *textures ) {
    GL_CHECK(glGenTextures(n, textures ));
}
void GL::deleteTextures(GLsizei n, const GLuint *textures) {
    GL_CHECK(glDeleteTextures(n, textures));
}
void GL::texParameteri(GLenum target, GLenum pname, GLint param ) {
    GL_CHECK(glTexParameteri(target, pname, param ));
}
void GL::texImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                    GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    GL_CHECK(glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels)); }

void GL::texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                       GLenum format, GLenum type, const GLvoid *pixels) {
    GL_CHECK(glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels)); }

void GL::generateMipmap(GLenum target) {
    GL_CHECK(glGenerateMipmap(target));
}

void GL::enableVertexAttribArray(GLuint index) {
    GL_CHECK(glEnableVertexAttribArray(index));
}
void GL::disableVertexAttribArray(GLuint index) {
    GL_CHECK(glDisableVertexAttribArray(index));
}
void GL::vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                             GLsizei stride, const void *pointer) {
    GL_CHECK(glVertexAttribPointer(index, size, type, normalized, stride, pointer));
}

void GL::drawArrays(GLenum mode, GLint first, GLsizei count ) {
    GL_CHECK(glDrawArrays(mode, first, count ));
}
void GL::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
    GL_CHECK(glDrawElements(mode, count, type, indices ));
}

void GL::uniform1f(GLint location, GLfloat v0) {
    GL_CHECK(glUniform1f(location, v0));
}
void GL::uniform2f(GLint location, GLfloat v0, GLfloat v1) {
    GL_CHECK(glUniform2f(location, v0, v1));
}
void GL::uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GL_CHECK(glUniform3f(location, v0, v1, v2));
}
void GL::uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GL_CHECK(glUniform4f(location, v0, v1, v2, v3));
}

void GL::uniform1i(GLint location, GLint v0) {
    GL_CHECK(glUniform1i(location, v0));
}
void GL::uniform2i(GLint location, GLint v0, GLint v1) {
    GL_CHECK(glUniform2i(location, v0, v1));
}
void GL::uniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    GL_CHECK(glUniform3i(location, v0, v1, v2));
}
void GL::uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GL_CHECK(glUniform4i(location, v0, v1, v2, v3));
}

void GL::uniform1fv(GLint location, GLsizei count, const GLfloat *value) {
    GL_CHECK(glUniform1fv(location, count, value));
}
void GL::uniform2fv(GLint location, GLsizei count, const GLfloat *value) {
    GL_CHECK(glUniform2fv(location, count, value));
}
void GL::uniform3fv(GLint location, GLsizei count, const GLfloat *value) {
    GL_CHECK(glUniform3fv(location, count, value));
}
void GL::uniform4fv(GLint location, GLsizei count, const GLfloat *value) {
    GL_CHECK(glUniform4fv(location, count, value));
}
void GL::uniform1iv(GLint location, GLsizei count, const GLint *value) {
    GL_CHECK(glUniform1iv(location, count, value));
}
void GL::uniform2iv(GLint location, GLsizei count, const GLint *value) {
    GL_CHECK(glUniform2iv(location, count, value));
}
void GL::uniform3iv(GLint location, GLsizei count, const GLint *value) {
    GL_CHECK(glUniform3iv(location, count, value));
}
void GL::uniform4iv(GLint location, GLsizei count, const GLint *value) {
    GL_CHECK(glUniform4iv(location, count, value));
}

void GL::uniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GL_CHECK(glUniformMatrix2fv(location, count, transpose, value));
}
void GL::uniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GL_CHECK(glUniformMatrix3fv(location, count, transpose, value));
}
void GL::uniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GL_CHECK(glUniformMatrix4fv(location, count, transpose, value));
}

// mapbuffer
void* GL::mapBuffer(GLenum target, GLenum access) {
    auto result =  glMapBuffer(target, access);
    GL_CHECK();
    return result;
}
GLboolean GL::unmapBuffer(GLenum target) {
    auto result = glUnmapBuffer(target);
    GL_CHECK();
    return result;
}

void GL::finish(void) {
    GL_CHECK(glFinish());
}

// VAO
void GL::bindVertexArray(GLuint array) {
    GL_CHECK(glBindVertexArray(array));
}
void GL::deleteVertexArrays(GLsizei n, const GLuint *arrays) {
    GL_CHECK(glDeleteVertexArrays(n, arrays));
}
void GL::genVertexArrays(GLsizei n, GLuint *arrays) {
    GL_CHECK(glGenVertexArrays(n, arrays));
}
}
