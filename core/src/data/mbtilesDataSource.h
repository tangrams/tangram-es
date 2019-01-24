#pragma once

#include "data/tileSource.h"

namespace SQLite {
class Database;
}


namespace Tangram {

class Platform;

struct MBTilesQueries;
class AsyncWorker;

class MBTilesDataSource : public TileSource::DataSource {
public:

    MBTilesDataSource(Platform& _platform, std::string _name, std::string _path, std::string _mime,
                      bool _cache = false, bool _offlineFallback = false);

    ~MBTilesDataSource();

    bool loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override;

    void clear() override {}

private:
    bool getTileData(const TileID& _tileId, std::vector<char>& _data);
    void storeTileData(const TileID& _tileId, const std::vector<char>& _data);
    bool loadNextSource(std::shared_ptr<TileTask> _task, TileTaskCb _cb);

    void openMBTiles();
    bool testSchema(SQLite::Database& db);
    void initSchema(SQLite::Database& db, std::string _name, std::string _mimeType);

    std::string m_name;

    // The path to an mbtiles tile store.
    std::string m_path;
    std::string m_mime;

    // Store tiles from next source
    bool m_cacheMode;

    // Offline fallback: Try next source (download) first, then fall back to mbtiles
    bool m_offlineMode;

    // Pointer to SQLite DB of MBTiles store
    std::unique_ptr<SQLite::Database> m_db;
    std::unique_ptr<MBTilesQueries> m_queries;
    std::unique_ptr<AsyncWorker> m_worker;

    // Platform reference
    Platform& m_platform;

    enum class Compression {
        undefined,
        identity,
        deflate,
        unsupported
    };

    struct {
        Compression compression = Compression::undefined;
        bool isCache = false;
        bool utfGrid = false;
    } m_schemaOptions;
};

}
