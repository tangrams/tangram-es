#pragma once

#include "platform.h"
#include <functional>

bool shouldRender();

void setRenderCallbackFunction(std::function<void()> callback);

void finishUrlRequests();
void initUrlRequests();

