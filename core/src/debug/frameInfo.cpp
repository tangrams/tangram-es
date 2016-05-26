#include "debug/frameInfo.h"

#include "tangram.h"
#include "debug/textDisplay.h"
#include "tile/tileManager.h"
#include "tile/tile.h"
#include "tile/tileCache.h"
#include "view/view.h"
#include "gl.h"
#include "gl/error.h"

#include <ctime>

#define TIME_TO_MS(start, end) (float(end - start) / CLOCKS_PER_SEC * 1000.0f)

namespace Tangram {

static float s_lastUpdateTime = 0.0;

static clock_t s_startFrameTime = 0,
    s_endFrameTime = 0,
    s_startUpdateTime = 0,
    s_endUpdateTime = 0;

void FrameInfo::beginUpdate() {

    if (Tangram::getDebugFlag(DebugFlags::tangram_infos)) {
        s_startUpdateTime = clock();
    }

}

void FrameInfo::endUpdate() {

    if (getDebugFlag(DebugFlags::tangram_infos)) {
        s_endUpdateTime = clock();
        s_lastUpdateTime = TIME_TO_MS(s_startUpdateTime, s_endUpdateTime);
    }

}

void FrameInfo::beginFrame() {

    if (getDebugFlag(DebugFlags::tangram_infos)) {
        s_startFrameTime = clock();
    }

}

void FrameInfo::draw(const View& _view, TileManager& _tileManager) {

    if (getDebugFlag(DebugFlags::tangram_infos)) {
        static int cpt = 0;

        clock_t endCpu = clock();
        static float timeCpu[60] = { 0 };
        static float timeUpdate[60] = { 0 };
        static float timeRender[60] = { 0 };
        timeCpu[cpt] = TIME_TO_MS(s_startFrameTime, endCpu);

        // Force opengl to finish commands (for accurate frame time)
        GL_CHECK(glFinish());

        s_endFrameTime = clock();
        timeRender[cpt] = TIME_TO_MS(s_startFrameTime, s_endFrameTime);

        if (++cpt == 60) { cpt = 0; }

        // Only compute average frame time every 60 frames
        float avgTimeRender = 0.f;
        float avgTimeCpu = 0.f;
        float avgTimeUpdate = 0.f;

        timeUpdate[cpt] = s_lastUpdateTime;

        for (int i = 0; i < 60; i++) {
            avgTimeRender += timeRender[i];
            avgTimeCpu += timeCpu[i];
            avgTimeUpdate += timeUpdate[i];
        }
        avgTimeRender /= 60;
        avgTimeCpu /= 60;
        avgTimeUpdate /= 60;

        size_t memused = 0;
        for (const auto& tile : _tileManager.getVisibleTiles()) {
            memused += tile->getMemoryUsage();
        }

        std::vector<std::string> debuginfos;

        debuginfos.push_back("visible tiles:"
                + std::to_string(_tileManager.getVisibleTiles().size()));
        debuginfos.push_back("tile cache size:"
                + std::to_string(_tileManager.getTileCache()->getMemoryUsage() / 1024) + "kb");
        debuginfos.push_back("tile size:" + std::to_string(memused / 1024) + "kb");
        debuginfos.push_back("avg frame cpu time:" + to_string_with_precision(avgTimeCpu, 2) + "ms");
        debuginfos.push_back("avg frame render time:" + to_string_with_precision(avgTimeRender, 2) + "ms");
        debuginfos.push_back("avg frame update time:" + to_string_with_precision(avgTimeUpdate, 2) + "ms");
        debuginfos.push_back("zoom:" + std::to_string(_view.getZoom()));
        debuginfos.push_back("pos:" + std::to_string(_view.getPosition().x) + "/"
                + std::to_string(_view.getPosition().y));
        debuginfos.push_back("tilt:" + std::to_string(_view.getPitch() * 57.3) + "deg");
        debuginfos.push_back("pixel scale:" + std::to_string(_view.pixelScale()));

        TextDisplay::Instance().draw(debuginfos);
    }

}

}
