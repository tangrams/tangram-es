#include "gl.h"

namespace Tangram {

GLenum GL::getError() {
    return 0;
}

const GLubyte* GL::getString(GLenum name) {
    return nullptr;
}

void GL::clear(GLbitfield mask) {
}
void GL::lineWidth(GLfloat width) {
}
void GL::viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
}

void GL::enable(GLenum id) {
}
void GL::disable(GLenum id) {
}
void GL::depthFunc(GLenum func) {
}
void GL::depthMask(GLboolean flag) {
}
void GL::depthRange(GLfloat n, GLfloat f) {
}
void GL::clearDepth(GLfloat d) {
}
void GL::blendFunc(GLenum sfactor, GLenum dfactor) {
}
void GL::stencilFunc(GLenum func, GLint ref, GLuint mask) {
}
void GL::stencilMask(GLuint mask) {
}
void GL::stencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
}
void GL::clearStencil(GLint s) {
}
void GL::colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
}
void GL::cullFace(GLenum mode) {
}
void GL::frontFace(GLenum mode) {
}
void GL::clearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
}
void GL::getIntegerv(GLenum pname, GLint *params ) {
}

// Program
void GL::useProgram(GLuint program) {
}
void GL::deleteProgram(GLuint program) {
}
void GL::deleteShader(GLuint shader) {
}
GLuint GL::createShader(GLenum type) {
    return 0;
}
GLuint GL::createProgram() {
    return 0;
}

void GL::compileShader(GLuint shader) {
}
void GL::attachShader(GLuint program, GLuint shader) {
}
void GL::linkProgram(GLuint program) {
}

void GL::shaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
}
void GL::getShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
}
void GL::getProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
}
GLint GL::getUniformLocation(GLuint program, const GLchar *name) {
    return 0;
}
GLint GL::getAttribLocation(GLuint program, const GLchar *name) {
    return 0;
}
void GL::getProgramiv(GLuint program, GLenum pname, GLint *params) {
}
void GL::getShaderiv(GLuint shader, GLenum pname, GLint *params) {
}

// Buffers
void GL::bindBuffer(GLenum target, GLuint buffer) {
}
void GL::deleteBuffers(GLsizei n, const GLuint *buffers) {
}
void GL::genBuffers(GLsizei n, GLuint *buffers) {
}
void GL::bufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
}
void GL::bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
}
void GL::readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, GLvoid* pixels) {
}

// Texture
void GL::bindTexture(GLenum target, GLuint texture ) {
}
void GL::activeTexture(GLenum texture) {
}
void GL::genTextures(GLsizei n, GLuint *textures ) {
}
void GL::deleteTextures(GLsizei n, const GLuint *textures) {
}
void GL::texParameteri(GLenum target, GLenum pname, GLint param ) {
}
void GL::texImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                    GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
}
void GL::texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                       GLenum format, GLenum type, const GLvoid *pixels) {
}
void GL::generateMipmap(GLenum target) {
}

void GL::enableVertexAttribArray(GLuint index) {
}
void GL::disableVertexAttribArray(GLuint index) {
}
void GL::vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                             GLsizei stride, const void *pointer) {
}

void GL::drawArrays(GLenum mode, GLint first, GLsizei count ) {
}
void GL::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
}

void GL::uniform1f(GLint location, GLfloat v0) {
}
void GL::uniform2f(GLint location, GLfloat v0, GLfloat v1) {
}
void GL::uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
}
void GL::uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
}

void GL::uniform1i(GLint location, GLint v0) {
}
void GL::uniform2i(GLint location, GLint v0, GLint v1) {
}
void GL::uniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
}
void GL::uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
}

void GL::uniform1fv(GLint location, GLsizei count, const GLfloat *value) {
}
void GL::uniform2fv(GLint location, GLsizei count, const GLfloat *value) {
}
void GL::uniform3fv(GLint location, GLsizei count, const GLfloat *value) {
}
void GL::uniform4fv(GLint location, GLsizei count, const GLfloat *value) {
}
void GL::uniform1iv(GLint location, GLsizei count, const GLint *value) {
}
void GL::uniform2iv(GLint location, GLsizei count, const GLint *value) {
}
void GL::uniform3iv(GLint location, GLsizei count, const GLint *value) {
}
void GL::uniform4iv(GLint location, GLsizei count, const GLint *value) {
}

void GL::uniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
}
void GL::uniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
}
void GL::uniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
}

// mapbuffer
void* GL::mapBuffer(GLenum target, GLenum access) {
    return nullptr;
}
GLboolean GL::unmapBuffer(GLenum target) {
    return true;
}

void GL::finish(void) {
}

// VAO
void GL::bindVertexArray(GLuint array) {
}
void GL::deleteVertexArrays(GLsizei n, const GLuint *arrays) {
}
void GL::genVertexArrays(GLsizei n, GLuint *arrays) {
}

// Framebuffer
void GL::bindFramebuffer(GLenum target, GLuint framebuffer) {
}
void GL::genFramebuffers(GLsizei n, GLuint *framebuffers) {
}
void GL::framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                              GLuint texture, GLint level) {
}
void GL::renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width,
                             GLsizei height) {
}
void GL::framebufferRenderbuffer(GLenum target, GLenum attachment,
                                 GLenum renderbuffertarget, GLuint renderbuffer) {
}
void GL::genRenderbuffers(GLsizei n, GLuint *renderbuffers) {
}
void GL::bindRenderbuffer(GLenum target, GLuint renderbuffer) {
}
void GL::deleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
}
void GL::deleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
}
GLenum GL::checkFramebufferStatus(GLenum target) {
    return 0;
}

}
