#pragma once

#include <string>

namespace Tangram {
namespace GLExtensions {

extern bool supportsMapBuffer;
extern bool supportsVAOs;
    
bool isAvailable(std::string _extension);
void load();
void printAvailableExtensions();

}
}
