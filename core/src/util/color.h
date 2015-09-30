#pragma once
#include <cstdint>

namespace Tangram {
    
struct Color {
    
    union {
        struct {
            uint8_t r, g, b, a;
        };
        uint32_t abgr = 0x00000000;
        // channel order appears reversed because of little-endianness
    };

};

}
