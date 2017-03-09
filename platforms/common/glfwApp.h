#pragma once

#include "platform.h"
#include "tangram.h"
#include <memory>

namespace Tangram {

namespace GlfwApp {

void create(std::shared_ptr<Platform> platform, std::string sceneFile, int width, int height);
void run();
void stop(int);
void destroy();

} // namespace GlfwApp

} // namespace Tangram
