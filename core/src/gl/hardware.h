#pragma once

#include <string>

namespace Tangram {
namespace Hardware {

extern bool supportsMapBuffer;
extern bool supportsVAOs;
extern bool supportsTextureNPOT;
extern uint32_t maxTextureSize;
extern uint32_t maxCombinedTextureUnits;

void loadCapabilities();
void loadExtensions();
bool isAvailable(std::string _extension);
void printAvailableExtensions();

}
}
