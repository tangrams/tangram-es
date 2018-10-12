#include "tile/tileTask.h"

#include "data/tileSource.h"
#include "scene/scene.h"
#include "tile/tile.h"
#include "tile/tileBuilder.h"
#include "util/mapProjection.h"


//stuff needed for terrain hack
#include <mutex>
#include <condition_variable>
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "stb_image.h"
#include "data/tileData.h"

namespace Tangram {


struct TerrainTileData {
    std::vector<float> data;
    size_t width = 0;
    size_t height = 0;
    float min = std::numeric_limits<float>::max();
    float max = std::numeric_limits<float>::min();
};


static float terrarium_to_float(const unsigned char *rgb_data) {
    const unsigned char red = rgb_data[0];
    const unsigned char green = rgb_data[1];
    const unsigned char blue = rgb_data[2];
    return (red * 256.0 + green + blue / 256.0) - 32768.0;
}

static void sync_url_request(const std::shared_ptr<Platform> &platform, const Url &url, UrlResponse &resp) {
    std::mutex requestMutex;
    std::condition_variable requestDoneCondition;
    bool requestDoneValue = false;

    auto urlCallback = [&](UrlResponse urlResp)->void {
        resp = std::move(urlResp);

        std::unique_lock<std::mutex> lock(requestMutex);
        requestDoneValue = true;
        requestDoneCondition.notify_one();
    };

    platform->startUrlRequest(url, urlCallback);
    std::unique_lock<std::mutex> lock(requestMutex);
    requestDoneCondition.wait(lock, [&]{ return requestDoneValue; });
}


static void stbi_image_free_wrapper(const unsigned char *imgData) {
    stbi_image_free(const_cast<unsigned char*>(imgData));
}

static std::shared_ptr<TerrainTileData> load_terrain_tile_data(const std::shared_ptr<Platform> &platform, const TileID tileId) {
    //load terrarium .png file from url
    //parse with stb_image
    //convert to float from terrarium and flip y axis

    std::string url = "https://elevation-tiles-prod.s3.amazonaws.com/terrarium/";
    url += std::to_string(tileId.z);
    url += "/";
    url += std::to_string(tileId.x);
    url += "/";
    url += std::to_string(tileId.y);
    url += ".png";

    Tangram::UrlResponse resp;
    sync_url_request(platform, url, resp);
    if(resp.error != nullptr) {
        return std::shared_ptr<TerrainTileData>();
    }

    int x = 0;
    int y = 0;
    int comp = 0;
    std::unique_ptr<const unsigned char, void(*)(const unsigned char*)> imgMem(stbi_load_from_memory(reinterpret_cast<const unsigned char *>(&resp.content[0]), resp.content.size(), &x, &y, &comp, 3), &stbi_image_free_wrapper);
    if(imgMem == nullptr || x <=0 || y <=0) {
        return std::shared_ptr<TerrainTileData>();
    }

    auto out = std::make_shared<TerrainTileData>();
    out->data.resize(x*y);
    out->width = x;
    out->height = y;

    float min = 0.0;
    float max = 0.0;
    for(size_t iy = 0; iy < y; iy++) {
        float *row_data = &out->data[iy*x];
        for(size_t ix = 0; ix < x; ix++) {
            //flip y-axis since polygon coordinates are upper-right quadrant
            const size_t imgMemOffset = (y-iy-1)*x*3 + ix*3;

            float h = terrarium_to_float(imgMem.get() + imgMemOffset);
            if(std::isnan(h) || std::isinf(h) || h < -10000 || h > 10000) {
                h = 0.0;
            }
            if(h < min) {
                min = h;
            }
            if(h > max) {
                max = h;
            }
            row_data[ix] = h;
        }
    }
    out->min = min;
    out->max = max;

    return out;
}

static double dem_based_z_offset(const TerrainTileData &dem, double tile_x, double tile_y, const TileID tileId) {
    if(dem.data.empty() || dem.height < 1 || dem.width < 1) {
        return 0.0;
    }

    const size_t px = glm::clamp(static_cast<size_t>(glm::round(glm::clamp(tile_x, 0.0, 1.0) * (dem.width-1))), static_cast<size_t>(0), dem.width-1);
    const size_t py = glm::clamp(static_cast<size_t>(glm::round(glm::clamp(tile_y, 0.0, 1.0) * (dem.height-1))), static_cast<size_t>(0), dem.height-1);

    return dem.data[py*dem.width + px];
}

static void transform_geometry_points(const TerrainTileData &dem, std::vector<Point> &points, const TileID& tileId, const BoundingBox &bbox) {
    const double w = glm::abs(bbox.width());

    //z_offset needs to be scaled by the size of the tile or by the zoom level
    //because the geometry of a low-zoom tile will be displayed bigger then that of the high-zoom tile
    const double z_offset_scale = 1.0 / w;

    for(Point &point: points) {
        const double z_offset = dem_based_z_offset(dem, point.x, point.y, tileId);
        point.z += z_offset * z_offset_scale;
    }
}

static void transform_geometry_lines(const TerrainTileData &dem, std::vector<Line> &lines, const TileID& tileId, const BoundingBox &bbox) {
    for(Line &line: lines) {
        transform_geometry_points(dem, line, tileId, bbox);
    }
}

static void transform_geometry_polygons(const TerrainTileData &dem, std::vector<Polygon> &polygons, const TileID& tileId, const BoundingBox &bbox) {
    for(Polygon &polygon: polygons) {
        transform_geometry_lines(dem, polygon, tileId, bbox);
    }
}

static void transform_geometry(const TerrainTileData &dem, std::shared_ptr<TileData> tileData, const TileID& tileId, const BoundingBox &bbox) {
    for(Layer &layer: tileData->layers) {
        for(Feature &feature : layer.features) {
            transform_geometry_polygons(dem, feature.polygons, tileId, bbox);
            transform_geometry_lines(dem, feature.lines, tileId, bbox);
            transform_geometry_points(dem, feature.points, tileId, bbox);
        }
    }
}

static void replace_raster_geometry(const std::shared_ptr<TileData> &tileData) {
    if(tileData->layers.empty() ||
       tileData->layers[0].features.empty() ||
       tileData->layers[0].features[0].geometryType != GeometryType::polygons) {
        assert(false); //raster tiles are supposed to only have a single feature with a rectangular polygon
        return;
    }

    std::vector<Polygon> &rasterPolygons = tileData->layers[0].features[0].polygons;
    rasterPolygons.clear();

    const int steps = 64;
    const double stepwidth = 1.0 / steps;

    rasterPolygons.reserve(steps*steps);
    for(int x=0; x < steps; x++) {
        const double xs = x*stepwidth;
        const double x1s = (x+1)*stepwidth;

        for(int y=0; y<steps; y++) {
            const double ys = y*stepwidth;
            const double y1s = (y+1)*stepwidth;

            Polygon p = {{
                {xs, ys, 0.0f},
                {x1s, ys, 0.0f},
                {x1s, y1s, 0.0f},
                {xs, y1s, 0.0f},
                {xs, ys, 0.0f}
            }};

            rasterPolygons.emplace_back(std::move(p));
        }
    }

}

static void postprocess_tileData(const std::shared_ptr<TileData> &tileData, const TileID tileId, const std::shared_ptr<TileSource> &source, TileBuilder& tileBuilder) {
    const auto &mapProjection = tileBuilder.scene().mapProjection();
    //auto bbox = mapProjection->TileLonLatBounds(m_tileId);
    auto bbox = mapProjection->TileBounds(tileId);


    auto platform = std::const_pointer_cast<Platform>(tileBuilder.scene().platform());
    if(!platform) {
        return;
    }
    auto dem = load_terrain_tile_data(platform, tileId);
    if(dem && !dem->data.empty() && (dem->min != 0.0 || dem->max != 0.0)) {
        if(source->isRaster()) {
            replace_raster_geometry(tileData);
        }
        transform_geometry(*dem, tileData, tileId, bbox);
    }
}



TileTask::TileTask(TileID& _tileId, std::shared_ptr<TileSource> _source, int _subTask) :
    m_tileId(_tileId),
    m_subTaskId(_subTask),
    m_source(_source),
    m_sourceGeneration(_source->generation()),
    m_ready(false),
    m_canceled(false),
    m_needsLoading(true),
    m_priority(0),
    m_proxyState(false) {}

TileTask::~TileTask() {}

std::unique_ptr<Tile> TileTask::getTile() {
    return std::move(m_tile);
}

void TileTask::setTile(std::unique_ptr<Tile>&& _tile) {
    m_tile = std::move(_tile);
    m_ready = true;
}

void TileTask::process(TileBuilder& _tileBuilder) {

    auto tileData = m_source->parse(*this, *_tileBuilder.scene().mapProjection());

    if (tileData) {
        postprocess_tileData(tileData, m_tileId, m_source, _tileBuilder);
        m_tile = _tileBuilder.build(m_tileId, *tileData, *m_source);
        m_ready = true;
    } else {
        cancel();
    }
}

void TileTask::complete() {

    for (auto& subTask : m_subTasks) {
        assert(subTask->isReady());
        subTask->complete(*this);
    }

}

}
