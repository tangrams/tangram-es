#include "data/rasterSource.h"
#include "data/propertyItem.h"
#include "data/tileData.h"
#include "scene/scene.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/mapProjection.h"
#include "platform.h"

namespace Tangram {

class RasterTileTask : public BinaryTileTask {
public:


    RasterTileTask(TileID& _tileId, Scene& _scene, TileSource& _source, int _subTask)
        : BinaryTileTask(_tileId, _scene, _source, _subTask) {
    }

    std::shared_ptr<Texture> m_texture;

    RasterSource* rasterSource() {
        auto scene = m_scene.lock();
        assert(scene);
        return reinterpret_cast<RasterSource*>(scene->getTileSource(m_sourceId));
    }

    bool hasData() const override {
        return bool(rawTileData) || bool(m_texture);
    }

    bool isReady() const override {
        if (!isSubTask()) {
            return bool(m_tile);
        } else {
            return bool(m_texture);
        }
    }

    void process(TileBuilder& _tileBuilder) override {
        auto source = rasterSource();
        if (!source) { return; }

        if (!m_texture) {
            // Decode texture data
            m_texture = source->createTexture(*rawTileData);
        }

        // Create tile geometries
        if (!isSubTask()) {
            BinaryTileTask::process(_tileBuilder);
        }
    }

    void complete() override {
        auto source = rasterSource();
        auto raster = source->getRaster(*this);
        assert(raster.isValid());

        m_tile->rasters().push_back(std::move(raster));

        for (auto& subTask : m_subTasks) {
            assert(subTask->isReady());
            subTask->complete(*this);
        }
    }

    void complete(TileTask& _mainTask) override {
        auto source = rasterSource();
        auto raster = source->getRaster(*this);
        assert(raster.isValid());

        _mainTask.tile()->rasters().push_back(std::move(raster));
    }


};


RasterSource::RasterSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
                           TextureOptions _options, TileSource::ZoomOptions _zoomOptions)
    : TileSource(_name, std::move(_sources), _zoomOptions),
      m_texOptions(_options) {

    m_emptyTexture = std::make_shared<Texture>(m_texOptions);
}

std::shared_ptr<Texture> RasterSource::createTexture(const std::vector<char>& _rawTileData) {
    if (_rawTileData.empty()) {
        return m_emptyTexture;
    }

    auto data = reinterpret_cast<const uint8_t*>(_rawTileData.data());
    auto length = _rawTileData.size();
    auto texture = std::make_shared<Texture>(data, length, m_texOptions);

    return texture;
}

std::shared_ptr<TileData> RasterSource::parse(const TileTask& _task) const {

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();

    Feature rasterFeature;
    rasterFeature.geometryType = GeometryType::polygons;
    rasterFeature.polygons = { { {
                                         {0.0f, 0.0f},
                                         {1.0f, 0.0f},
                                         {1.0f, 1.0f},
                                         {0.0f, 1.0f},
                                         {0.0f, 0.0f}
                                 } } };
    rasterFeature.props = Properties();

    tileData->layers.emplace_back("");
    tileData->layers.back().features.push_back(rasterFeature);
    return tileData;

}

std::shared_ptr<TileTask> RasterSource::createTask(Scene& _scene, TileID _tileId, int _subTask) {
    auto task = std::make_shared<RasterTileTask>(_tileId, _scene, *this, _subTask);

    task->cb = [](std::shared_ptr<TileTask> task) {
        auto scene = task->scene();
        if (!scene) { return; }

        if (task->isSubTask() && !task->hasData()) {
            auto& t = static_cast<RasterTileTask&>(*task);
            t.m_texture = scene->emptyTexture();
        } else if (task->hasData()) {
            scene->tileWorker()->enqueue(task);
        }

        if (task->isReady()) {
            scene->requestRender();
        } else {
            task->cancel();
        }
    };

    createSubTasks(_scene, *task);

    // First try existing textures cache
    {
        TileID id(_tileId.x, _tileId.y, _tileId.z);

        auto texIt = m_textures.find(id);
        if (texIt != m_textures.end()) {
            task->m_texture = texIt->second;

            // No more loading needed.
            task->startedLoading();

            return task;
        }
    }

    return task;
}

Raster RasterSource::getRaster(const TileTask& _task) {
    const auto& taskTileID = _task.tileId();
    TileID id(taskTileID.x, taskTileID.y, taskTileID.z);

    auto texIt = m_textures.find(id);
    if (texIt != m_textures.end()) {
        return { id, texIt->second };
    }

    auto& task = static_cast<const RasterTileTask&>(_task);
    m_textures.emplace(id, task.m_texture);

    return { id, task.m_texture };
}

void RasterSource::clearRasters() {
    for (auto& raster: m_rasterSources) {
        raster->clearRasters();
    }

    m_textures.clear();
}

void RasterSource::clearRaster(const TileID &tileID) {
    TileID id(tileID.x, tileID.y, tileID.z);

    for (auto& raster: m_rasterSources) {
        TileID rasterID = id.withMaxSourceZoom(raster->maxZoom());
        raster->clearRaster(rasterID);
    }

    auto rasterID = id.withMaxSourceZoom(m_zoomOptions.maxZoom);

    // We do not want to delete the texture reference from the
    // DS if any of the tiles is still using this as a reference
    if (m_textures[rasterID].use_count() <= 1) {
        m_textures.erase(rasterID);
    }
}

}
