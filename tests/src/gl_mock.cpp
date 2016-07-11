#include "gl.h"

extern "C" {

    GLenum glGetError( void ){ return 0; }
    const GLubyte* glGetString(GLenum name){ return nullptr; }

    void glEnable(GLenum){}
    void glDisable(GLenum){}
    void glDepthFunc(GLenum func){}
    void glDepthMask(GLboolean flag){}

#ifdef PLATFORM_OSX
    void glDepthRange(GLclampd n, GLclampd f){}
    void glClearDepth(GLclampd d){}
#else
    void glDepthRangef(GLfloat n, GLfloat f){}
    void glClearDepthf(GLfloat d){}
#endif

    void glBlendFunc(GLenum sfactor, GLenum dfactor){}
    void glStencilFunc(GLenum func, GLint ref, GLuint mask){}
    void glStencilMask(GLuint mask){}
    void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass){}
    void glClearStencil(GLint s){}
    void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){}
    void glCullFace(GLenum mode){}
    void glFrontFace(GLenum mode){}
    void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){}
    void glUseProgram(GLuint program){}

    void glClear( GLbitfield mask ){}
    void glViewport( GLint x, GLint y, GLsizei width, GLsizei height ){}
    void glLineWidth( GLfloat width ){}

    void glDeleteProgram (GLuint program) {}
    void glDeleteShader (GLuint shader) {}

    GLuint glCreateShader (GLenum type) { return 0; }
    GLuint glCreateProgram () { return 0; }
    void glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length){}
    void glGetShaderiv (GLuint shader, GLenum pname, GLint *params){}
    void glCompileShader (GLuint shader){}
    void glAttachShader (GLuint program, GLuint shader){}
    void glLinkProgram (GLuint program){}
    void glDrawArrays( GLenum mode, GLint first, GLsizei count ){}
    void glDrawElements( GLenum mode, GLsizei count,
                         GLenum type, const GLvoid *indices ){}

    void glEnableVertexAttribArray (GLuint index){}
    void glDisableVertexAttribArray (GLuint index){}
    void glEnableVertexArrayAttrib (GLuint vaobj, GLuint index){}
    void glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer){}

    void glGetProgramiv (GLuint program, GLenum pname, GLint *params){}
    void glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog){}
    void glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog){}
    GLint glGetUniformLocation (GLuint program, const GLchar *name){ return 0; }
    GLint glGetAttribLocation (GLuint program, const GLchar *name){ return 0; }

    void glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage) {}
    void glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data){}

    void glGetBooleanv( GLenum pname, GLboolean *params ){}
    void glGetDoublev( GLenum pname, GLdouble *params ){}
    void glGetFloatv( GLenum pname, GLfloat *params ){}
    void glGetIntegerv( GLenum pname, GLint *params ){}
    void glBindTexture( GLenum target, GLuint texture ){}
    void glActiveTexture (GLenum texture){}
    void glGenTextures( GLsizei n, GLuint *textures ){}

    void glDeleteTextures( GLsizei n, const GLuint *textures){}

    void glTexParameterf( GLenum target, GLenum pname, GLfloat param ){}
    void glTexParameteri( GLenum target, GLenum pname, GLint param ){}
    void glGenerateMipmap (GLenum target){}
    void glTexImage2D( GLenum target, GLint level,
                       GLint internalFormat,
                       GLsizei width, GLsizei height,
                       GLint border, GLenum format, GLenum type,
                       const GLvoid *pixels ){}

    void glTexSubImage2D( GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLsizei width, GLsizei height,
                          GLenum format, GLenum type,
                          const GLvoid *pixels ){}

    void glBindBuffer (GLenum target, GLuint buffer){}
    void glDeleteBuffers (GLsizei n, const GLuint *buffers){}
    void glGenBuffers (GLsizei n, GLuint *buffers){}
    void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum format, GLenum type, GLvoid* pixels){}

    void glUniform1f (GLint location, GLfloat v0){}
    void glUniform2f (GLint location, GLfloat v0, GLfloat v1){}
    void glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2){}
    void glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3){}
    void glUniform1i (GLint location, GLint v0){}
    void glUniform2i (GLint location, GLint v0, GLint v1){}
    void glUniform3i (GLint location, GLint v0, GLint v1, GLint v2){}
    void glUniform4i (GLint location, GLint v0, GLint v1, GLint v2, GLint v3){}
    void glUniform1fv (GLint location, GLsizei count, const GLfloat *value){}
    void glUniform2fv (GLint location, GLsizei count, const GLfloat *value){}
    void glUniform3fv (GLint location, GLsizei count, const GLfloat *value){}
    void glUniform4fv (GLint location, GLsizei count, const GLfloat *value){}
    void glUniform1iv (GLint location, GLsizei count, const GLint *value){}
    void glUniform2iv (GLint location, GLsizei count, const GLint *value){}
    void glUniform3iv (GLint location, GLsizei count, const GLint *value){}
    void glUniform4iv (GLint location, GLsizei count, const GLint *value){}
    void glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){}
    void glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){}
    void glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){}
    void glValidateProgram (GLuint program){}
    void glVertexAttrib1d (GLuint index, GLdouble x){}
    void glVertexAttrib1dv (GLuint index, const GLdouble *v){}
    void glVertexAttrib1f (GLuint index, GLfloat x){}
    void glVertexAttrib1fv (GLuint index, const GLfloat *v){}
    void glVertexAttrib1s (GLuint index, GLshort x){}
    void glVertexAttrib1sv (GLuint index, const GLshort *v){}
    void glVertexAttrib2d (GLuint index, GLdouble x, GLdouble y){}
    void glVertexAttrib2dv (GLuint index, const GLdouble *v){}
    void glVertexAttrib2f (GLuint index, GLfloat x, GLfloat y){}
    void glVertexAttrib2fv (GLuint index, const GLfloat *v){}
    void glVertexAttrib2s (GLuint index, GLshort x, GLshort y){}
    void glVertexAttrib2sv (GLuint index, const GLshort *v){}
    void glVertexAttrib3d (GLuint index, GLdouble x, GLdouble y, GLdouble z){}
    void glVertexAttrib3dv (GLuint index, const GLdouble *v){}
    void glVertexAttrib3f (GLuint index, GLfloat x, GLfloat y, GLfloat z){}
    void glVertexAttrib3fv (GLuint index, const GLfloat *v){}
    void glVertexAttrib3s (GLuint index, GLshort x, GLshort y, GLshort z){}
    void glVertexAttrib3sv (GLuint index, const GLshort *v){}
    void glVertexAttrib4Nbv (GLuint index, const GLbyte *v){}
    void glVertexAttrib4Niv (GLuint index, const GLint *v){}
    void glVertexAttrib4Nsv (GLuint index, const GLshort *v){}
    void glVertexAttrib4Nub (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w){}
    void glVertexAttrib4Nubv (GLuint index, const GLubyte *v){}
    void glVertexAttrib4Nuiv (GLuint index, const GLuint *v){}
    void glVertexAttrib4Nusv (GLuint index, const GLushort *v){}
    void glVertexAttrib4bv (GLuint index, const GLbyte *v){}
    void glVertexAttrib4d (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w){}
    void glVertexAttrib4dv (GLuint index, const GLdouble *v){}
    void glVertexAttrib4f (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w){}
    void glVertexAttrib4fv (GLuint index, const GLfloat *v){}
    void glVertexAttrib4iv (GLuint index, const GLint *v){}
    void glVertexAttrib4s (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w){}
    void glVertexAttrib4sv (GLuint index, const GLshort *v){}
    void glVertexAttrib4ubv (GLuint index, const GLubyte *v){}
    void glVertexAttrib4uiv (GLuint index, const GLuint *v){}
    void glVertexAttrib4usv (GLuint index, const GLushort *v){}
    void glFinish(void){}

    // mapbuffer
    void* glMapBuffer(GLenum target, GLenum access){ return nullptr; }
    GLboolean glUnmapBuffer(GLenum target){ return false; }

    // VAO
    void glBindVertexArray (GLuint array){}
    void glDeleteVertexArrays (GLsizei n, const GLuint *arrays){}
    void glGenVertexArrays (GLsizei n, GLuint *arrays){}

}
