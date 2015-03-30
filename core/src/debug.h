#pragma once

typedef unsigned long flag_t;

#define TANGRAM_FREEZE_TILES 0x1 // While on, the set of tiles currently being drawn will not update to match the view
#define TANGRAM_PROXY_COLORS 0x2 // Applies a color change to every other zoom level of tiles to visualize proxy tile behavior
#define TANGRAM_TILE_BOUNDS  0x4 // Draws tile boundaries
