#pragma once

namespace Tangram {

class RenderState;
class TileManager;
class View;

struct FrameInfo {

    static void beginUpdate();
    static void beginFrame();

    static void endUpdate();

    static void draw(RenderState& rs, const View& _view, const TileManager& _tileManager);
};

}
