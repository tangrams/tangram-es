#pragma once

#include "platform.h"
#include <functional>
#include <Evas_GL.h>

bool shouldRender();

void setRenderCallbackFunction(std::function<void()> callback);

void initUrlRequests(const char* proxyAddress);
void stopUrlRequests();

void setEvasGlAPI(Evas_GL_API *glApi);
