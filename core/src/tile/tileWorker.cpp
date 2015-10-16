#include "tileWorker.h"

#include "platform.h"
#include "tile/tile.h"
#include "view/view.h"
#include "scene/scene.h"
#include "scene/styleContext.h"
#include "tile/tileManager.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"
#include "data/propertyItem.h"

#include <algorithm>

#define WORKER_NICENESS 10

namespace Tangram {

TileWorker::TileWorker(TileManager& _tileManager, int _num_worker)
    : m_tileManager(_tileManager) {
    m_running = true;

    for (int i = 0; i < _num_worker; i++) {
        m_workers.emplace_back(&TileWorker::run, this);
    }
}

TileWorker::~TileWorker(){
    if (m_running) {
        stop();
    }
}

void TileWorker::run() {

    setCurrentThreadPriority(WORKER_NICENESS);

    StyleContext context;

    while (true) {

        std::shared_ptr<TileTask> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_condition.wait(lock, [&, this]{
                    return !m_running || !m_queue.empty();
                });

            // Check if thread should stop
            if (!m_running) {
                break;
            }

            // Remove all canceled tasks
            auto removes = std::remove_if(m_queue.begin(), m_queue.end(),
                [](const auto& a) { return a->tile->isCanceled(); });

            m_queue.erase(removes, m_queue.end());

            if (m_queue.empty()) {
                continue;
            }

            // Pop highest priority tile from queue
            auto it = std::min_element(m_queue.begin(), m_queue.end(),
                [](const auto& a, const auto& b) {
                    if (a->tile->isVisible() != b->tile->isVisible()) {
                        return a->tile->isVisible();
                    }
                    return a->tile->getPriority() < b->tile->getPriority();
                });

            task = std::move(*it);
            m_queue.erase(it);
        }

        if (task->tile->isCanceled()) {
            continue;
        }

        auto tileData = task->process();

        Feature f;
        // f.lines.push_back({
        //         {0.f,0.f,0.f},
        //         {0.f,1.f,0.f},
        //         {1.f,1.f,0.f},
        //         {1.f,0.f,0.f},
        //         {0.f,0.f,0.f}
        //     });

        tileData->layers.push_back({"grid"});

        for (float y= 0; y < 1; y+=0.05) {
            Line line_x;
            Line line_y;
            for (float x= 0; x < 1; x+=0.05) {
                line_x.push_back({x,y,0.f});
                line_y.push_back({y,x,0.f});

            }
            f.lines.push_back(std::move(line_x));
            f.lines.push_back(std::move(line_y));
        }
        f.geometryType = GeometryType::lines;
        tileData->layers.back().features.push_back(f);

        f.lines.clear();
        Line line_x;
        Line line_y;
        for (float x= 0; x < 1; x+=0.05) {
            line_x.push_back({x,0.f,0.f});
            line_y.push_back({0.f,x,0.f});

        }
        f.lines.push_back(std::move(line_x));
        f.lines.push_back(std::move(line_y));
        f.props.add("border","yes");
        tileData->layers.back().features.push_back(f);

        f.lines.clear();
        f.props.clear();
        f.props.add("poly","yes");
        int tx = task->tile->getID().x;
        int ty = task->tile->getID().y;
        f.props.add("color", tx % 2 == 0
                    ? (ty % 2 == 0 ? "green" : "blue")
                    : (ty % 2 == 0 ? "red" : "yellow"));

        f.geometryType = GeometryType::polygons;
        for (float y= 0; y < 1; y+=0.05) {

            for (float x= 0; x < 1; x+=0.05) {
                Line line;

                line.push_back({x,y,0.f});
                line.push_back({x+0.05,y,0.f});
                line.push_back({x+0.05,y+0.05,0.f});
                line.push_back({x,y+0.05,0.f});
                line.push_back({x,y,0.f});

                f.polygons.push_back({std::move(line)});
            }
        }
        tileData->layers.back().features.push_back({f});

        f.polygons.clear();
        f.props.clear();
        f.props.add("point","yes");
        f.props.add("name",task->tile->getID().toString());
        f.geometryType = GeometryType::points;
        f.points.push_back({0.5f,0.5f,0.f});
        tileData->layers.back().features.push_back({f});

        // NB: Save shared reference to Scene while building tile
        auto scene = m_tileManager.getScene();

        context.initFunctions(*scene);

        if (tileData) {
            task->tile->build(context, *scene, *tileData, *task->source);
        }

        m_tileManager.tileProcessed(std::move(task));
        requestRender();
    }
}

void TileWorker::enqueue(std::shared_ptr<TileTask>&& task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_running) {
            return;
        }
        m_queue.push_back(std::move(task));
    }
    m_condition.notify_one();
}

void TileWorker::stop() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_running = false;
    }

    m_condition.notify_all();

    for (std::thread &worker: m_workers) {
        worker.join();
    }
}

}
