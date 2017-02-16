#include "platform_gl.h"

ELEMENTARY_GLVIEW_GLOBAL_DEFINE()

namespace Tangram {

GLenum GL::getError() {
    return __evas_gl_glapi->glGetError();
}

const GLubyte* GL::getString(GLenum name) {
    return __evas_gl_glapi->glGetString(name);
}

void GL::clear(GLbitfield mask) {
    __evas_gl_glapi->glClear(mask);
}
void GL::lineWidth(GLfloat width) {
    __evas_gl_glapi->glLineWidth(width);
}
void GL::viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    __evas_gl_glapi->glViewport(x, y, width, height);
}

void GL::enable(GLenum id) {
    __evas_gl_glapi->glEnable(id);
}
void GL::disable(GLenum id) {
    __evas_gl_glapi->glDisable(id);
}
void GL::depthFunc(GLenum func) {
    __evas_gl_glapi->glDepthFunc(func);
}
void GL::depthMask(GLboolean flag) {
    __evas_gl_glapi->glDepthMask(flag);
}
void GL::depthRange(GLfloat n, GLfloat f) {
    __evas_gl_glapi->glDepthRangef(n, f);
}
void GL::clearDepth(GLfloat d) {
    __evas_gl_glapi->glClearDepthf(d);
}
void GL::blendFunc(GLenum sfactor, GLenum dfactor) {
    __evas_gl_glapi->glBlendFunc(sfactor, dfactor);
}
void GL::stencilFunc(GLenum func, GLint ref, GLuint mask) {
    __evas_gl_glapi->glStencilFunc(func, ref, mask);
}
void GL::stencilMask(GLuint mask) {
    __evas_gl_glapi->glStencilMask(mask);
}
void GL::stencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    __evas_gl_glapi->glStencilOp(fail, zfail, zpass);
}
void GL::clearStencil(GLint s) {
    __evas_gl_glapi->glClearStencil(s);
}
void GL::colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    __evas_gl_glapi->glColorMask(red, green, blue, alpha);
}
void GL::cullFace(GLenum mode) {
    __evas_gl_glapi->glCullFace(mode);
}
void GL::frontFace(GLenum mode) {
    __evas_gl_glapi->glFrontFace(mode);
}
void GL::clearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    __evas_gl_glapi->glClearColor(red, green, blue, alpha);
}
void GL::getIntegerv(GLenum pname, GLint *params ) {
    __evas_gl_glapi->glGetIntegerv(pname, params );
}

// Program
void GL::useProgram(GLuint program) {
    __evas_gl_glapi->glUseProgram(program);
}
void GL::deleteProgram(GLuint program) {
    __evas_gl_glapi->glDeleteProgram(program);
}
void GL::deleteShader(GLuint shader) {
    __evas_gl_glapi->glDeleteShader(shader);
}
GLuint GL::createShader(GLenum type) {
    return __evas_gl_glapi->glCreateShader(type);
}
GLuint GL::createProgram() {
    return __evas_gl_glapi->glCreateProgram();
}

void GL::compileShader(GLuint shader) {
    __evas_gl_glapi->glCompileShader(shader);
}
void GL::attachShader(GLuint program, GLuint shader) {
    __evas_gl_glapi->glAttachShader(program,shader);
}
void GL::linkProgram(GLuint program) {
    __evas_gl_glapi->glLinkProgram(program);
}

void GL::shaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
    auto source = const_cast<const GLchar**>(string);
    __evas_gl_glapi->glShaderSource(shader, count, source, length);
}
void GL::getShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    __evas_gl_glapi->glGetShaderInfoLog(shader, bufSize, length, infoLog);
}
void GL::getProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    __evas_gl_glapi->glGetProgramInfoLog(program, bufSize, length, infoLog);
}
GLint GL::getUniformLocation(GLuint program, const GLchar *name) {
    return __evas_gl_glapi->glGetUniformLocation(program, name);
}
GLint GL::getAttribLocation(GLuint program, const GLchar *name) {
    return __evas_gl_glapi->glGetAttribLocation(program, name);
}
void GL::getProgramiv(GLuint program, GLenum pname, GLint *params) {
    __evas_gl_glapi->glGetProgramiv(program,pname,params);
}
void GL::getShaderiv(GLuint shader, GLenum pname, GLint *params) {
    __evas_gl_glapi->glGetShaderiv(shader,pname, params);
}

// Buffers
void GL::bindBuffer(GLenum target, GLuint buffer) {
    __evas_gl_glapi->glBindBuffer(target, buffer);
}
void GL::deleteBuffers(GLsizei n, const GLuint *buffers) {
    __evas_gl_glapi->glDeleteBuffers(n, buffers);
}
void GL::genBuffers(GLsizei n, GLuint *buffers) {
    __evas_gl_glapi->glGenBuffers(n, buffers);
}
void GL::bufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    __evas_gl_glapi->glBufferData(target, size, data, usage);
}
void GL::bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    __evas_gl_glapi->glBufferSubData(target, offset, size, data);
}
void GL::readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, GLvoid* pixels) {
    __evas_gl_glapi->glReadPixels(x, y, width, height, format, type, pixels);
}

// Texture
void GL::bindTexture(GLenum target, GLuint texture ) {
    __evas_gl_glapi->glBindTexture(target, texture );
}
void GL::activeTexture(GLenum texture) {
    __evas_gl_glapi->glActiveTexture(texture);
}
void GL::genTextures(GLsizei n, GLuint *textures ) {
    __evas_gl_glapi->glGenTextures(n, textures );
}
void GL::deleteTextures(GLsizei n, const GLuint *textures) {
    __evas_gl_glapi->glDeleteTextures(n, textures);
}
void GL::texParameteri(GLenum target, GLenum pname, GLint param ) {
    __evas_gl_glapi->glTexParameteri(target, pname, param );
}
void GL::texImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                    GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    __evas_gl_glapi->glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels); }

void GL::texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                       GLenum format, GLenum type, const GLvoid *pixels) {
    __evas_gl_glapi->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels); }

void GL::generateMipmap(GLenum target) {
    __evas_gl_glapi->glGenerateMipmap(target);
}

void GL::enableVertexAttribArray(GLuint index) {
    __evas_gl_glapi->glEnableVertexAttribArray(index);
}
void GL::disableVertexAttribArray(GLuint index) {
    __evas_gl_glapi->glDisableVertexAttribArray(index);
}
void GL::vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                             GLsizei stride, const void *pointer) {
    __evas_gl_glapi->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void GL::drawArrays(GLenum mode, GLint first, GLsizei count ) {
    __evas_gl_glapi->glDrawArrays(mode, first, count );
}
void GL::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
    __evas_gl_glapi->glDrawElements(mode, count, type, indices );
}

void GL::uniform1f(GLint location, GLfloat v0) {
    __evas_gl_glapi->glUniform1f(location, v0);
}
void GL::uniform2f(GLint location, GLfloat v0, GLfloat v1) {
    __evas_gl_glapi->glUniform2f(location, v0, v1);
}
void GL::uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    __evas_gl_glapi->glUniform3f(location, v0, v1, v2);
}
void GL::uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    __evas_gl_glapi->glUniform4f(location, v0, v1, v2, v3);
}

void GL::uniform1i(GLint location, GLint v0) {
    __evas_gl_glapi->glUniform1i(location, v0);
}
void GL::uniform2i(GLint location, GLint v0, GLint v1) {
    __evas_gl_glapi->glUniform2i(location, v0, v1);
}
void GL::uniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    __evas_gl_glapi->glUniform3i(location, v0, v1, v2);
}
void GL::uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    __evas_gl_glapi->glUniform4i(location, v0, v1, v2, v3);
}

void GL::uniform1fv(GLint location, GLsizei count, const GLfloat *value) {
    __evas_gl_glapi->glUniform1fv(location, count, value);
}
void GL::uniform2fv(GLint location, GLsizei count, const GLfloat *value) {
    __evas_gl_glapi->glUniform2fv(location, count, value);
}
void GL::uniform3fv(GLint location, GLsizei count, const GLfloat *value) {
    __evas_gl_glapi->glUniform3fv(location, count, value);
}
void GL::uniform4fv(GLint location, GLsizei count, const GLfloat *value) {
    __evas_gl_glapi->glUniform4fv(location, count, value);
}
void GL::uniform1iv(GLint location, GLsizei count, const GLint *value) {
    __evas_gl_glapi->glUniform1iv(location, count, value);
}
void GL::uniform2iv(GLint location, GLsizei count, const GLint *value) {
    __evas_gl_glapi->glUniform2iv(location, count, value);
}
void GL::uniform3iv(GLint location, GLsizei count, const GLint *value) {
    __evas_gl_glapi->glUniform3iv(location, count, value);
}
void GL::uniform4iv(GLint location, GLsizei count, const GLint *value) {
    __evas_gl_glapi->glUniform4iv(location, count, value);
}

void GL::uniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    __evas_gl_glapi->glUniformMatrix2fv(location, count, transpose, value);
}
void GL::uniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    __evas_gl_glapi->glUniformMatrix3fv(location, count, transpose, value);
}
void GL::uniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    __evas_gl_glapi->glUniformMatrix4fv(location, count, transpose, value);
}

// mapbuffer
void* GL::mapBuffer(GLenum target, GLenum access) {
    return __evas_gl_glapi->glMapBufferOES(target, access);
}
GLboolean GL::unmapBuffer(GLenum target) {
    return __evas_gl_glapi->glUnmapBufferOES(target);
}

void GL::finish(void) {
    __evas_gl_glapi->glFinish();
}

// VAO
void GL::bindVertexArray(GLuint array) {
    __evas_gl_glapi->glBindVertexArrayOES(array);
}
void GL::deleteVertexArrays(GLsizei n, const GLuint *arrays) {
    __evas_gl_glapi->glDeleteVertexArraysOES(n, arrays);
}
void GL::genVertexArrays(GLsizei n, GLuint *arrays) {
    __evas_gl_glapi->glGenVertexArraysOES(n, arrays);
}

// Framebuffer
void GL::bindFramebuffer(GLenum target, GLuint framebuffer) {
    __evas_gl_glapi->glBindFramebuffer(target, framebuffer);
}

void GL::genFramebuffers(GLsizei n, GLuint *framebuffers) {
    __evas_gl_glapi->glGenFramebuffers(n, framebuffers);
}

void GL::framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                              GLuint texture, GLint level) {
    __evas_gl_glapi->glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void GL::renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width,
                             GLsizei height) {
    __evas_gl_glapi->glRenderbufferStorage(target, internalformat, width, height);
}

void GL::framebufferRenderbuffer(GLenum target, GLenum attachment,
                                 GLenum renderbuffertarget, GLuint renderbuffer) {
    __evas_gl_glapi->glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

void GL::genRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    __evas_gl_glapi->glGenRenderbuffers(n, renderbuffers);
}

void GL::bindRenderbuffer(GLenum target, GLuint renderbuffer) {
    __evas_gl_glapi->glBindRenderbuffer(target, renderbuffer);
}

void GL::deleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
    __evas_gl_glapi->glDeleteFramebuffers(n, framebuffers);
}

void GL::deleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
    __evas_gl_glapi->glDeleteRenderbuffers(n, renderbuffers);
}

GLenum GL::checkFramebufferStatus(GLenum target) {
    return __evas_gl_glapi->glCheckFramebufferStatus(target);
}

}
