#include "data/rasterSource.h"
#include "data/propertyItem.h"
#include "data/tileData.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/mapProjection.h"
#include "log.h"

namespace Tangram {

class RasterTileTask : public BinaryTileTask {
public:
    RasterTileTask(TileID& _tileId, std::shared_ptr<TileSource> _source, int _subTask)
        : BinaryTileTask(_tileId, _source, _subTask) {}

    std::shared_ptr<Texture> m_texture;

    std::shared_ptr<RasterSource> rasterSource() {
        return reinterpret_cast<std::weak_ptr<RasterSource>*>(&m_source)->lock();
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
            m_texture = source->createTexture(m_tileId, *rawTileData);
        }

        // Create tile geometries
        if (!isSubTask()) {
            BinaryTileTask::process(_tileBuilder);
        }
    }

    void complete() override {
        auto source = rasterSource();
        if (!source) { return; }

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
        if (!source) { return; }

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

std::shared_ptr<Texture> RasterSource::createTexture(TileID _tile, const std::vector<char>& _rawTileData) {
    if (_rawTileData.empty()) {
        return m_emptyTexture;
    }

    auto data = reinterpret_cast<const uint8_t*>(_rawTileData.data());
    auto length = _rawTileData.size();

    return std::make_shared<Texture>(data, length, m_texOptions);
}

void RasterSource::loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) {
    // TODO, remove this
    // Overwrite cb to set empty texture on failure
    TileTaskCb cb{[this, _cb](std::shared_ptr<TileTask> _task) {

            if (!_task->hasData()) {
                auto& task = static_cast<RasterTileTask&>(*_task);
                task.m_texture = m_emptyTexture;
            }
            _cb.func(_task);
        }};

    TileSource::loadTileData(_task, cb);
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

std::shared_ptr<TileTask> RasterSource::createTask(TileID _tileId, int _subTask) {
    auto task = std::make_shared<RasterTileTask>(_tileId, shared_from_this(), _subTask);

    createSubTasks(task);

    // First try existing textures cache
    TileID id(_tileId.x, _tileId.y, _tileId.z);

    auto texIt = m_textures.find(id);
    if (texIt != m_textures.end()) {
        task->m_texture = texIt->second;

        if (task->m_texture) {
            // No more loading needed.
            task->startedLoading();
            return task;
        }
    }

    return task;
}

Raster RasterSource::getRaster(const RasterTileTask& _task) {
    const auto& tileId = _task.tileId();
    TileID id(tileId.x, tileId.y, tileId.z);

    auto entry = m_textures.emplace(id, _task.m_texture);
    std::shared_ptr<Texture> texture = entry.first->second;
    LOGD("Cache: add %d/%d/%d - reused: %d", id.x, id.y, id.z, !entry.second);

    // Remove textures that are only held by cache
    for(auto it = m_textures.begin(), end = m_textures.end(); it != end; ) {
        if (it->second.use_count() == 1) {
            LOGD("Cache: remove %d/%d/%d", it->first.x, it->first.y, it->first.z);
            it = m_textures.erase(it);
        } else {
            ++it;
        }
    }

    return Raster{id, texture};
}

}
