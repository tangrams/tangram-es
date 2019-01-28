#pragma once

#include "tangram.h"
#include <memory>

namespace Tangram {

namespace GlfwApp {

void create(std::unique_ptr<Platform> platform, int width, int height);
void setScene(const std::string& _path, const std::string& _yaml);
void loadSceneFile(bool setPosition = false);
void parseArgs(int argc, char* argv[]);
void setWireframeMode(bool state);
void run();
void stop(int);
void destroy();

extern std::string sceneFile;
extern std::string sceneYaml;
extern std::string apiKey;

} // namespace GlfwApp

} // namespace Tangram
