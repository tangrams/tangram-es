#pragma once

#if defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
#define DESKTOP_GL true
#else
#define DESKTOP_GL false
#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_RPI)
typedef long GLsizeiptr;
typedef long GLintptr;
#else
#include <stddef.h>
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
#endif

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

/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef void            GLvoid;
typedef signed char     GLbyte;     /* 1-byte signed */
typedef short           GLshort;    /* 2-byte signed */
typedef int             GLint;      /* 4-byte signed */
typedef unsigned char   GLubyte;    /* 1-byte unsigned */
typedef unsigned short  GLushort;   /* 2-byte unsigned */
typedef unsigned int    GLuint;     /* 4-byte unsigned */
typedef int             GLsizei;    /* 4-byte signed */
typedef float           GLfloat;    /* single precision float */
typedef float           GLclampf;   /*single precision float in [0,1] */
typedef double          GLdouble;   /* double precision float */
typedef double          GLclampd;   /* double precision float in [0,1] */
typedef char            GLchar;

/* Utility */
#define GL_VENDOR                       0x1F00
#define GL_RENDERER                     0x1F01
#define GL_VERSION                      0x1F02
#define GL_EXTENSIONS                   0x1F03

/* Boolean values */
#define GL_FALSE                        0
#define GL_TRUE                         1

#define GL_DEPTH_BUFFER_BIT             0x00000100
#define GL_STENCIL_BUFFER_BIT           0x00000400
#define GL_COLOR_BUFFER_BIT             0x00004000

/* Errors */
#define GL_NO_ERROR                     0
#define GL_INVALID_ENUM                 0x0500
#define GL_INVALID_VALUE                0x0501
#define GL_INVALID_OPERATION            0x0502
#define GL_OUT_OF_MEMORY                0x0505
#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506

/* Data types */
#define GL_BYTE                         0x1400
#define GL_UNSIGNED_BYTE                0x1401
#define GL_SHORT                        0x1402
#define GL_UNSIGNED_SHORT               0x1403
#define GL_INT                          0x1404
#define GL_UNSIGNED_INT                 0x1405
#define GL_FLOAT                        0x1406
#define GL_2_BYTES                      0x1407
#define GL_3_BYTES                      0x1408
#define GL_4_BYTES                      0x1409
#define GL_DOUBLE                       0x140A

/* Primitives */
#define GL_POINTS                       0x0000
#define GL_LINES                        0x0001
#define GL_LINE_LOOP                    0x0002
#define GL_LINE_STRIP                   0x0003
#define GL_TRIANGLES                    0x0004
#define GL_TRIANGLE_STRIP               0x0005
#define GL_TRIANGLE_FAN                 0x0006
#define GL_QUADS                        0x0007
#define GL_QUAD_STRIP                   0x0008
#define GL_POLYGON                      0x0009

/* Blending */
#define GL_BLEND                        0x0BE2
#define GL_BLEND_SRC                    0x0BE1
#define GL_BLEND_DST                    0x0BE0
#define GL_ZERO                         0
#define GL_ONE                          1
#define GL_SRC_COLOR                    0x0300
#define GL_ONE_MINUS_SRC_COLOR          0x0301
#define GL_SRC_ALPHA                    0x0302
#define GL_ONE_MINUS_SRC_ALPHA          0x0303
#define GL_DST_ALPHA                    0x0304
#define GL_ONE_MINUS_DST_ALPHA          0x0305
#define GL_DST_COLOR                    0x0306
#define GL_ONE_MINUS_DST_COLOR          0x0307
#define GL_SRC_ALPHA_SATURATE           0x0308

/* Buffers, Pixel Drawing/Reading */
#define GL_NONE                         0
#define GL_RED                          0x1903
#define GL_GREEN                        0x1904
#define GL_BLUE                         0x1905
#define GL_ALPHA                        0x1906
#define GL_LUMINANCE                    0x1909
#define GL_LUMINANCE_ALPHA              0x190A
#define GL_ALPHA_BITS                   0x0D55
#define GL_RED_BITS                     0x0D52
#define GL_GREEN_BITS                   0x0D53
#define GL_BLUE_BITS                    0x0D54
#define GL_INDEX_BITS                   0x0D51
#define GL_READ_BUFFER                  0x0C02
#define GL_DRAW_BUFFER                  0x0C01
#define GL_STEREO                       0x0C33
#define GL_BITMAP                       0x1A00
#define GL_COLOR                        0x1800
#define GL_DEPTH                        0x1801
#define GL_STENCIL                      0x1802
#define GL_RGB                          0x1907
#define GL_RGBA                         0x1908
#define GL_RGBA8_OES                    0x8058

#define GL_NEAREST                      0x2600
#define GL_LINEAR                       0x2601
#define GL_NEAREST_MIPMAP_NEAREST       0x2700
#define GL_LINEAR_MIPMAP_NEAREST        0x2701
#define GL_NEAREST_MIPMAP_LINEAR        0x2702
#define GL_LINEAR_MIPMAP_LINEAR         0x2703

#define GL_CLAMP_TO_EDGE                0x812F
#define GL_CLAMP                        0x2900
#define GL_REPEAT                       0x2901

/* texture_cube_map */
#define GL_NORMAL_MAP                   0x8511
#define GL_REFLECTION_MAP               0x8512
#define GL_TEXTURE_CUBE_MAP             0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP     0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X  0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X  0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y  0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y  0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z  0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z  0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP       0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE    0x851C

/* Polygons */
#define GL_POINT                        0x1B00
#define GL_LINE                         0x1B01
#define GL_FILL                         0x1B02
#define GL_CW                           0x0900
#define GL_CCW                          0x0901
#define GL_FRONT                        0x0404
#define GL_BACK                         0x0405
#define GL_EDGE_FLAG                    0x0B43
#define GL_CULL_FACE                    0x0B44
#define GL_CULL_FACE_MODE               0x0B45
#define GL_FRONT_FACE                   0x0B46

/* Depth buffer */
#define GL_NEVER                        0x0200
#define GL_LESS                         0x0201
#define GL_EQUAL                        0x0202
#define GL_LEQUAL                       0x0203
#define GL_GREATER                      0x0204
#define GL_NOTEQUAL                     0x0205
#define GL_GEQUAL                       0x0206
#define GL_ALWAYS                       0x0207
#define GL_DEPTH_TEST                   0x0B71
#define GL_DEPTH_BITS                   0x0D56
#define GL_DEPTH_CLEAR_VALUE            0x0B73
#define GL_DEPTH_FUNC                   0x0B74
#define GL_DEPTH_RANGE                  0x0B70
#define GL_DEPTH_WRITEMASK              0x0B72
#define GL_DEPTH_COMPONENT              0x1902
#define GL_DEPTH_COMPONENT16            0x81A5

/* Stencil */
#define GL_STENCIL_BITS                 0x0D57
#define GL_STENCIL_TEST                 0x0B90
#define GL_STENCIL_CLEAR_VALUE          0x0B91
#define GL_STENCIL_FUNC                 0x0B92
#define GL_STENCIL_VALUE_MASK           0x0B93
#define GL_STENCIL_FAIL                 0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL      0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS      0x0B96
#define GL_STENCIL_REF                  0x0B97
#define GL_STENCIL_WRITEMASK            0x0B98
#define GL_STENCIL_INDEX                0x1901
#define GL_KEEP                         0x1E00
#define GL_REPLACE                      0x1E01
#define GL_INCR                         0x1E02
#define GL_DECR                         0x1E03

/* Texture mapping */
#define GL_NEAREST_MIPMAP_NEAREST       0x2700
#define GL_NEAREST_MIPMAP_LINEAR        0x2702
#define GL_LINEAR_MIPMAP_NEAREST        0x2701
#define GL_LINEAR_MIPMAP_LINEAR         0x2703
#define GL_NEAREST                      0x2600
#define GL_TEXTURE0                     0x84C0
#define GL_TEXTURE_2D                   0x0DE1
#define GL_TEXTURE_WRAP_S               0x2802
#define GL_TEXTURE_WRAP_T               0x2803
#define GL_TEXTURE_MAG_FILTER           0x2800
#define GL_TEXTURE_MIN_FILTER           0x2801

/* Framebuffers, Render buffers */
#define GL_FRAMEBUFFER                  0x8D40
#define GL_RENDERBUFFER                 0x8D41
#define GL_RENDERBUFFER_WIDTH           0x8D42
#define GL_RENDERBUFFER_HEIGHT          0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#define GL_RENDERBUFFER_RED_SIZE        0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE      0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE       0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE      0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE      0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE    0x8D55
#define GL_COLOR_ATTACHMENT0            0x8CE0
#define GL_DEPTH_ATTACHMENT             0x8D00
#define GL_STENCIL_ATTACHMENT           0x8D20
#define GL_NONE                         0
#define GL_FRAMEBUFFER_COMPLETE         0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
#define GL_FRAMEBUFFER_UNSUPPORTED      0x8CDD
#define GL_FRAMEBUFFER_BINDING          0x8CA6
#define GL_RENDERBUFFER_BINDING         0x8CA7
#define GL_MAX_RENDERBUFFER_SIZE        0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506

// glext.h
#define GL_ARRAY_BUFFER                 0x8892
#define GL_ELEMENT_ARRAY_BUFFER         0x8893
#define GL_ARRAY_BUFFER_BINDING         0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_STATIC_DRAW                  0x88E4
#define GL_DYNAMIC_DRAW                 0x88E8

// Program
#define GL_FRAGMENT_SHADER              0x8B30
#define GL_VERTEX_SHADER                0x8B31
#define GL_COMPILE_STATUS               0x8B81
#define GL_LINK_STATUS                  0x8B82
#define GL_INFO_LOG_LENGTH              0x8B84

// mapbuffer
#define GL_READ_ONLY                    0x88B8
#define GL_WRITE_ONLY                   0x88B9
#define GL_READ_WRITE                   0x88BA

#define GL_MAX_TEXTURE_SIZE             0x0D33
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D

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

    // Framebuffers
    GL_APICALL void GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer);
    GL_APICALL void GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint *framebuffers);
    GL_APICALL void GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment,
                        GLenum textarget, GLuint texture, GLint level);
    GL_APICALL void GL_APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat,
                        GLsizei width, GLsizei height);
    GL_APICALL void GL_APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment,
                        GLenum renderbuffertarget, GLuint renderbuffer);
    GL_APICALL void GL_APIENTRY glGenRenderbuffers(GLsizei n, GLuint *renderbuffers);
    GL_APICALL void GL_APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer);
    GL_APICALL void GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers);
    GL_APICALL void GL_APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers);
    GL_APICALL GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target);

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
