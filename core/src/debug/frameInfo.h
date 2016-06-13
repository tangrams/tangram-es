#pragma once

namespace Tangram {

class TileManager;
class View;

struct FrameInfo {

    static void beginUpdate();
    static void beginFrame();

    static void endUpdate();

    static void draw(const View& _view, TileManager& _tileManager, float _pixelsPerPoint);
};

}
