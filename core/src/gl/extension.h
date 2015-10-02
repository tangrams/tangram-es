#pragma once

#include <string>

namespace Tangram {
namespace GLExtensions {

extern bool supportsMapBuffer;
    
bool isAvailable(std::string _extension);
void load(bool _log = false);

}
}
