#pragma once

namespace Tangram {

enum DebugFlags {
    freeze_tiles = 0,   // While on, the set of tiles currently being drawn will not update to match the view
    proxy_colors,       // Applies a color change to every other zoom level of tiles to visualize proxy tile behavior
    tile_bounds,        // Draws tile boundaries
    tile_infos,         // Debug tile infos
    labels,             // Debug label bounding boxes
    tangram_infos,      // Various text tangram debug info printed on the screen
    draw_all_labels,    // Draw all labels
    tangram_stats,      // Tangram frame graph stats
    selection_buffer,   // Render selection framebuffer
};

// Set debug features on or off using a boolean (see debug.h)
void setDebugFlag(DebugFlags _flag, bool _on);

// Get the boolean state of a debug feature (see debug.h)
bool getDebugFlag(DebugFlags _flag);

// Toggle the boolean state of a debug feature (see debug.h)
void toggleDebugFlag(DebugFlags _flag);

}
