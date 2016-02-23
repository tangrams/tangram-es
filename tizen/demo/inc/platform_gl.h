#pragma once

#include "gl.h"
#include <Evas_GL.h>

#ifdef ELEMENTARY_GLVIEW_GLOBAL_DEFINE
ELEMENTARY_GLVIEW_GLOBAL_DECLARE()
#define GL()
#else
extern Evas_GL_API *__evas_gl_glapi;

#define ELEMENTARY_GLVIEW_GLOBAL_DEFINE() \
    Evas_GL_API *__evas_gl_glapi = NULL;
#define ELEMENTARY_GLVIEW_GLOBAL_USE(glview) \
   do { __evas_gl_glapi = elm_glview_gl_api_get(glview); } while (0)
#define GL() __evas_gl_glapi->
#endif
