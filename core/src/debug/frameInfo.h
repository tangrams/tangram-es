#pragma once

namespace Tangram {

class RenderState;
class TileManager;
class View;

namespace Debug {

struct FrameInfo {
#ifdef TANGRAM_DEBUG_RENDERER
    static void beginUpdate();
    static void beginFrame();
    static void endUpdate();
    static void draw(RenderState& rs, const View& _view, const TileManager& _tileManager);
#else
    static void beginUpdate(){}
    static void beginFrame(){}
    static void endUpdate(){}
    static void draw(RenderState&, const View&, TileManager&){}
#endif
};
}
}
