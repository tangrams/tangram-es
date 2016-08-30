#pragma once

#include "platform.h"
#include <functional>
#include <Evas_GL.h>

bool shouldRender();

void setRenderCallbackFunction(std::function<void()> callback);

void finishUrlRequests();
void initUrlRequests(char* proxyAddress);
void deinitUrlRequests();
void setEvasGlAPI(Evas_GL_API *glApi);
