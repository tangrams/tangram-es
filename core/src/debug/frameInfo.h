#pragma once

#include <memory>

namespace Tangram {

class TileManager;
class View;


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
