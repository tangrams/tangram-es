#pragma once

#include "map.h"
#include "platform.h"
#include <memory>

namespace Tangram {

namespace GlfwApp {

void create(std::shared_ptr<Platform> platform, int width, int height);
void setScene(const std::string& _path, const std::string& _yaml);
void parseArgs(int argc, char* argv[]);
void run();
void stop(int);
void destroy();

} // namespace GlfwApp

} // namespace Tangram
