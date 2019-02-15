#include "debug/frameInfo.h"

#include "debug/textDisplay.h"
#include "gl.h"
#include "gl/glError.h"
#include "gl/primitives.h"
#include "map.h"
#include "tile/tileManager.h"
#include "tile/tile.h"
#include "tile/tileCache.h"
#include "view/view.h"

#include <deque>
#include <ctime>

#define TIME_TO_MS(start, end) (float(end - start) / CLOCKS_PER_SEC * 1000.0f)

#define DEBUG_STATS_MAX_SIZE 128

namespace Tangram {

static float s_lastUpdateTime = 0.0;

static clock_t s_startFrameTime = 0,
    s_endFrameTime = 0,
    s_startUpdateTime = 0,
    s_endUpdateTime = 0;

void FrameInfo::beginUpdate() {

    if (getDebugFlag(DebugFlags::tangram_infos) || getDebugFlag(DebugFlags::tangram_stats)) {
        s_startUpdateTime = clock();
    }

}

void FrameInfo::endUpdate() {

    if (getDebugFlag(DebugFlags::tangram_infos) || getDebugFlag(DebugFlags::tangram_stats)) {
        s_endUpdateTime = clock();
        s_lastUpdateTime = TIME_TO_MS(s_startUpdateTime, s_endUpdateTime);
    }

}

void FrameInfo::beginFrame() {

    if (getDebugFlag(DebugFlags::tangram_infos) || getDebugFlag(DebugFlags::tangram_stats)) {
        s_startFrameTime = clock();
    }

}


void FrameInfo::draw(RenderState& rs, const View& _view, const TileManager& _tileManager) {

    if (getDebugFlag(DebugFlags::tangram_infos) || getDebugFlag(DebugFlags::tangram_stats)) {
        static int cpt = 0;

        static std::deque<float> updatetime;
        static std::deque<float> rendertime;

        clock_t endCpu = clock();
        static float timeCpu[60] = { 0 };
        static float timeUpdate[60] = { 0 };
        static float timeRender[60] = { 0 };
        timeCpu[cpt] = TIME_TO_MS(s_startFrameTime, endCpu);

        if (updatetime.size() >= DEBUG_STATS_MAX_SIZE) {
            updatetime.pop_front();
        }
        if (rendertime.size() >= DEBUG_STATS_MAX_SIZE) {
            rendertime.pop_front();
        }

        rendertime.push_back(timeRender[cpt]);
        updatetime.push_back(timeUpdate[cpt]);

        // Force opengl to finish commands (for accurate frame time)
        GL::finish();

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
        size_t features = 0;
        for (const auto& tile : _tileManager.getVisibleTiles()) {
            memused += tile->getMemoryUsage();
            features += tile->getSelectionFeatures().size();
        }

        if (getDebugFlag(DebugFlags::tangram_infos)) {
            std::vector<std::string> debuginfos;

            debuginfos.push_back("visible tiles:"
                                 + std::to_string(_tileManager.getVisibleTiles().size()));
            debuginfos.push_back("selectable features:"
                                 + std::to_string(features));
            debuginfos.push_back("tile cache size:"
                                 + std::to_string(_tileManager.getTileCache()->getMemoryUsage() / 1024) + "kb");
            debuginfos.push_back("tile size:" + std::to_string(memused / 1024) + "kb");
            debuginfos.push_back("avg frame cpu time:" + to_string_with_precision(avgTimeCpu, 2) + "ms");
            debuginfos.push_back("avg frame render time:" + to_string_with_precision(avgTimeRender, 2) + "ms");
            debuginfos.push_back("avg frame update time:" + to_string_with_precision(avgTimeUpdate, 2) + "ms");
            debuginfos.push_back("zoom:" + std::to_string(_view.getZoom()));
            debuginfos.push_back("pos:" + std::to_string(_view.getPosition().x) + "/"
                                 + std::to_string(_view.getPosition().y));
            auto center = _view.getCenterCoordinates();
            debuginfos.push_back("LngLat:" + std::to_string(center.longitude) + ", " + std::to_string(center.latitude));
            debuginfos.push_back("tilt:" + std::to_string(_view.getPitch() * 57.3) + "deg");
            debuginfos.push_back("pixel scale:" + std::to_string(_view.pixelScale()));

            TextDisplay::Instance().draw(rs, debuginfos);
        }

        if (getDebugFlag(DebugFlags::tangram_stats)) {
            const int scale = 5 * _view.pixelScale();

            for (size_t i = 0; i < updatetime.size(); i++) {
                float tupdate = updatetime[i] * scale;
                float trender = rendertime[i] * scale;
                float offsetx = i * 4 * _view.pixelScale();

                Primitives::setColor(rs, 0xfff000);
                Primitives::drawLine(rs, glm::vec2(offsetx, 0), glm::vec2(offsetx, tupdate));
                Primitives::setColor(rs, 0x0000ff);
                Primitives::drawLine(rs, glm::vec2(offsetx, tupdate), glm::vec2(offsetx, tupdate + trender));
            }

            // Draw 16.6ms horizontal line
            Primitives::setColor(rs, 0xff0000);
            Primitives::drawLine(rs, glm::vec2(0.0, 16.6 * scale),
                glm::vec2(DEBUG_STATS_MAX_SIZE * 4 * _view.pixelScale() + 4, 16.6 * scale));
        }
    }
}

}
