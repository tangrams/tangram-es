#pragma once

#include <memory>

namespace Tangram {

class TileManager;
class View;

enum DebugFlags {
    freeze_tiles = 0,   // While on, the set of tiles currently being drawn will not update to match the view
    proxy_colors,       // Applies a color change to every other zoom level of tiles to visualize proxy tile behavior
    tile_bounds,        // Draws tile boundaries
    tile_infos,         // Debug tile infos
    labels,             // Debug label bounding boxes
    tangram_infos,      // Various text tangram debug info printed on the screen
    all_labels          // Draw all labels
};

namespace Debug {

void beginUpdate();
void beginFrame();

void endUpdate();
void endFrame();

struct Context {
    Context(std::shared_ptr<TileManager> tileManager, std::shared_ptr<View> view)
        : tileManager(tileManager), view(view)
    {}

    std::shared_ptr<TileManager> tileManager;
    std::shared_ptr<View> view;
};

#define TIME_TO_MS(start, end) (float(end - start) / CLOCKS_PER_SEC * 1000.0f)

extern std::unique_ptr<Context> context;

}

}
