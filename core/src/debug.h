#pragma once

namespace Tangram {
    
    enum DebugFlags {
        FREEZE_TILES = 0,    // While on, the set of tiles currently being drawn will not update to match the view
        PROXY_COLORS,        // Applies a color change to every other zoom level of tiles to visualize proxy tile behavior
        TILE_BOUNDS,         // Draws tile boundaries
        TILE_INFOS           // Debug tile infos
    };
    
}
