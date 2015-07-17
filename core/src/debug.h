#pragma once

namespace Tangram {

    enum DebugFlags {
        freeze_tiles = 0,   // While on, the set of tiles currently being drawn will not update to match the view
        proxy_colors,       // Applies a color change to every other zoom level of tiles to visualize proxy tile behavior
        tile_bounds,        // Draws tile boundaries
        tile_infos,         // Debug tile infos
        labels              // Debug label bounding boxes
    };

}
