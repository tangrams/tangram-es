#include "platform_gl.h"

ELEMENTARY_GLVIEW_GLOBAL_DEFINE()

GLenum __glGetError() {
    return __evas_gl_glapi->glGetError();
}

const GLubyte* __glGetString(GLenum name) {
    return __evas_gl_glapi->glGetString(name);
}

void __glClear(GLbitfield mask) {
    __evas_gl_glapi->glClear(mask);
}
void __glLineWidth(GLfloat width) {
    __evas_gl_glapi->glLineWidth(width);
}
void __glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    __evas_gl_glapi->glViewport(x, y, width, height);
}

void __glEnable(GLenum id) {
    __evas_gl_glapi->glEnable(id);
}
void __glDisable(GLenum id) {
    __evas_gl_glapi->glDisable(id);
}
void __glDepthFunc(GLenum func) {
    __evas_gl_glapi->glDepthFunc(func);
}
void __glDepthMask(GLboolean flag) {
    __evas_gl_glapi->glDepthMask(flag);
}
void __glDepthRangef(GLfloat n, GLfloat f) {
    __evas_gl_glapi->glDepthRangef(n, f);
}
void __glClearDepthf(GLfloat d) {
    __evas_gl_glapi->glClearDepthf(d);
}
void __glBlendFunc(GLenum sfactor, GLenum dfactor) {
    __evas_gl_glapi->glBlendFunc(sfactor, dfactor);
}
void __glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    __evas_gl_glapi->glStencilFunc(func, ref, mask);
}
void __glStencilMask(GLuint mask) {
    __evas_gl_glapi->glStencilMask(mask);
}
void __glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    __evas_gl_glapi->glStencilOp(fail, zfail, zpass);
}
void __glClearStencil(GLint s) {
    __evas_gl_glapi->glClearStencil(s);
}
void __glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    __evas_gl_glapi->glColorMask(red, green, blue, alpha);
}
void __glCullFace(GLenum mode) {
    __evas_gl_glapi->glCullFace(mode);
}
void __glFrontFace(GLenum mode) {
    __evas_gl_glapi->glFrontFace(mode);
}
void __glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    __evas_gl_glapi->glClearColor(red, green, blue, alpha);
}
void __glGetIntegerv(GLenum pname, GLint *params ) {
    __evas_gl_glapi->glGetIntegerv(pname, params );
}

// Program
void __glUseProgram(GLuint program) {
    __evas_gl_glapi->glUseProgram(program);
}
void __glDeleteProgram(GLuint program) {
    __evas_gl_glapi->glDeleteProgram(program);
}
void __glDeleteShader(GLuint shader) {
    __evas_gl_glapi->glDeleteShader(shader);
}
GLuint __glCreateShader(GLenum type) {
    return __evas_gl_glapi->glCreateShader(type);
}
GLuint __glCreateProgram() {
    return __evas_gl_glapi->glCreateProgram();
}

void __glCompileShader(GLuint shader) {
    __evas_gl_glapi->glCompileShader(shader);
}
void __glAttachShader(GLuint program, GLuint shader) {
    __evas_gl_glapi->glAttachShader(program,shader);
}
void __glLinkProgram(GLuint program) {
    __evas_gl_glapi->glLinkProgram(program);
}

void __glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
	auto source = const_cast<const GLchar**>(string);
    __evas_gl_glapi->glShaderSource(shader, count, source, length);
}
void __glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    __evas_gl_glapi->glGetShaderInfoLog(shader, bufSize, length, infoLog);
}
void __glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    __evas_gl_glapi->glGetProgramInfoLog(program, bufSize, length, infoLog);
}
GLint __glGetUniformLocation(GLuint program, const GLchar *name) {
    return __evas_gl_glapi->glGetUniformLocation(program, name);
}
GLint __glGetAttribLocation(GLuint program, const GLchar *name) {
    return __evas_gl_glapi->glGetAttribLocation(program, name);
}
void __glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
    __evas_gl_glapi->glGetProgramiv(program,pname,params);
}
void __glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
    __evas_gl_glapi->glGetShaderiv(shader,pname, params);
}

// Buffers
void __glBindBuffer(GLenum target, GLuint buffer) {
    __evas_gl_glapi->glBindBuffer(target, buffer);
}
void __glDeleteBuffers(GLsizei n, const GLuint *buffers) {
    __evas_gl_glapi->glDeleteBuffers(n, buffers);
}
void __glGenBuffers(GLsizei n, GLuint *buffers) {
    __evas_gl_glapi->glGenBuffers(n, buffers);
}
void __glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    __evas_gl_glapi->glBufferData(target, size, data, usage);
}
void __glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    __evas_gl_glapi->glBufferSubData(target, offset, size, data);
}
void __glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, GLvoid* pixels) {
    __evas_gl_glapi->glReadPixels(x, y, width, height, format, type, pixels);
}

// Texture
void __glBindTexture(GLenum target, GLuint texture ) {
    __evas_gl_glapi->glBindTexture(target, texture );
}
void __glActiveTexture(GLenum texture) {
    __evas_gl_glapi->glActiveTexture(texture);
}
void __glGenTextures(GLsizei n, GLuint *textures ) {
    __evas_gl_glapi->glGenTextures(n, textures );
}
void __glDeleteTextures(GLsizei n, const GLuint *textures) {
    __evas_gl_glapi->glDeleteTextures(n, textures);
}
void __glTexParameteri(GLenum target, GLenum pname, GLint param ) {
    __evas_gl_glapi->glTexParameteri(target, pname, param );
}
void __glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                    GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    __evas_gl_glapi->glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels); }

void __glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                       GLenum format, GLenum type, const GLvoid *pixels) {
    __evas_gl_glapi->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels); }

void __glGenerateMipmap(GLenum target) {
    __evas_gl_glapi->glGenerateMipmap(target);
}

void __glEnableVertexAttribArray(GLuint index) {
    __evas_gl_glapi->glEnableVertexAttribArray(index);
}
void __glDisableVertexAttribArray(GLuint index) {
    __evas_gl_glapi->glDisableVertexAttribArray(index);
}
void __glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                             GLsizei stride, const void *pointer) {
    __evas_gl_glapi->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void __glDrawArrays(GLenum mode, GLint first, GLsizei count ) {
    __evas_gl_glapi->glDrawArrays(mode, first, count );
}
void __glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
    __evas_gl_glapi->glDrawElements(mode, count, type, indices );
}

void __glUniform1f(GLint location, GLfloat v0) {
    __evas_gl_glapi->glUniform1f(location, v0);
}
void __glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    __evas_gl_glapi->glUniform2f(location, v0, v1);
}
void __glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    __evas_gl_glapi->glUniform3f(location, v0, v1, v2);
}
void __glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    __evas_gl_glapi->glUniform4f(location, v0, v1, v2, v3);
}

void __glUniform1i(GLint location, GLint v0) {
    __evas_gl_glapi->glUniform1i(location, v0);
}
void __glUniform2i(GLint location, GLint v0, GLint v1) {
    __evas_gl_glapi->glUniform2i(location, v0, v1);
}
void __glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    __evas_gl_glapi->glUniform3i(location, v0, v1, v2);
}
void __glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    __evas_gl_glapi->glUniform4i(location, v0, v1, v2, v3);
}

void __glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
    __evas_gl_glapi->glUniform1fv(location, count, value);
}
void __glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
    __evas_gl_glapi->glUniform2fv(location, count, value);
}
void __glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
    __evas_gl_glapi->glUniform3fv(location, count, value);
}
void __glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
    __evas_gl_glapi->glUniform4fv(location, count, value);
}
void __glUniform1iv(GLint location, GLsizei count, const GLint *value) {
    __evas_gl_glapi->glUniform1iv(location, count, value);
}
void __glUniform2iv(GLint location, GLsizei count, const GLint *value) {
    __evas_gl_glapi->glUniform2iv(location, count, value);
}
void __glUniform3iv(GLint location, GLsizei count, const GLint *value) {
    __evas_gl_glapi->glUniform3iv(location, count, value);
}
void __glUniform4iv(GLint location, GLsizei count, const GLint *value) {
    __evas_gl_glapi->glUniform4iv(location, count, value);
}

void __glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    __evas_gl_glapi->glUniformMatrix2fv(location, count, transpose, value);
}
void __glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    __evas_gl_glapi->glUniformMatrix3fv(location, count, transpose, value);
}
void __glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    __evas_gl_glapi->glUniformMatrix4fv(location, count, transpose, value);
}

// mapbuffer
void* __glMapBufferOES(GLenum target, GLenum access) {
    return __evas_gl_glapi->glMapBufferOES(target, access);
}
GLboolean __glUnmapBufferOES(GLenum target) {
    return __evas_gl_glapi->glUnmapBufferOES(target);
}

void __glFinish(void) {
    __evas_gl_glapi->glFinish();
}

// VAO
void __glBindVertexArray(GLuint array) {
    __evas_gl_glapi->glBindVertexArrayOES(array);
}
void __glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {
    __evas_gl_glapi->glDeleteVertexArraysOES(n, arrays);
}
void __glGenVertexArrays(GLsizei n, GLuint *arrays) {
    __evas_gl_glapi->glGenVertexArraysOES(n, arrays);
}

