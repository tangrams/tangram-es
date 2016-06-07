#pragma once

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <emscripten/emscripten.h>

#include "platform.h"

void processNetworkQueue();

void finishUrlRequests();