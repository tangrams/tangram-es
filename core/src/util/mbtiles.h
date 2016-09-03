#pragma once

#include "log.h"

#include <SQLiteCpp/Database.h>
#include <data/geoJsonSource.h>
#include <data/mvtSource.h>
#include <data/topoJsonSource.h>

namespace Tangram {
namespace MBTiles {

/**
 * The schema.sql used to set up an MBTiles Database.
 *
 * https://github.com/mapbox/node-mbtiles/blob/4bbfaf991969ce01c31b95184c4f6d5485f717c3/lib/schema.sql
 */
const char* SCHEMA = R"SQL_ESC(BEGIN;

CREATE TABLE IF NOT EXISTS map (
   zoom_level INTEGER,
   tile_column INTEGER,
   tile_row INTEGER,
   tile_id TEXT,
   grid_id TEXT
);

CREATE TABLE IF NOT EXISTS grid_key (
    grid_id TEXT,
    key_name TEXT
);

CREATE TABLE IF NOT EXISTS keymap (
    key_name TEXT,
    key_json TEXT
);

CREATE TABLE IF NOT EXISTS grid_utfgrid (
    grid_id TEXT,
    grid_utfgrid BLOB
);

CREATE TABLE IF NOT EXISTS images (
    tile_data blob,
    tile_id text
);

CREATE TABLE IF NOT EXISTS metadata (
    name text,
    value text
);

CREATE TABLE IF NOT EXISTS geocoder_data (
    type TEXT,
    shard INTEGER,
    data BLOB
);

CREATE UNIQUE INDEX IF NOT EXISTS map_index ON map (zoom_level, tile_column, tile_row);
CREATE UNIQUE INDEX IF NOT EXISTS grid_key_lookup ON grid_key (grid_id, key_name);
CREATE UNIQUE INDEX IF NOT EXISTS keymap_lookup ON keymap (key_name);
CREATE UNIQUE INDEX IF NOT EXISTS grid_utfgrid_lookup ON grid_utfgrid (grid_id);
CREATE UNIQUE INDEX IF NOT EXISTS images_id ON images (tile_id);
CREATE UNIQUE INDEX IF NOT EXISTS name ON metadata (name);
CREATE INDEX IF NOT EXISTS map_grid_id ON map (grid_id);
CREATE INDEX IF NOT EXISTS geocoder_type_index ON geocoder_data (type);
CREATE UNIQUE INDEX IF NOT EXISTS geocoder_shard_index ON geocoder_data (type, shard);

CREATE VIEW IF NOT EXISTS tiles AS
    SELECT
        map.zoom_level AS zoom_level,
        map.tile_column AS tile_column,
        map.tile_row AS tile_row,
        images.tile_data AS tile_data
    FROM map
    JOIN images ON images.tile_id = map.tile_id;

CREATE VIEW IF NOT EXISTS grids AS
    SELECT
        map.zoom_level AS zoom_level,
        map.tile_column AS tile_column,
        map.tile_row AS tile_row,
        grid_utfgrid.grid_utfgrid AS grid
    FROM map
    JOIN grid_utfgrid ON grid_utfgrid.grid_id = map.grid_id;

CREATE VIEW IF NOT EXISTS grid_data AS
    SELECT
        map.zoom_level AS zoom_level,
        map.tile_column AS tile_column,
        map.tile_row AS tile_row,
        keymap.key_name AS key_name,
        keymap.key_json AS key_json
    FROM map
    JOIN grid_key ON map.grid_id = grid_key.grid_id
    JOIN keymap ON grid_key.key_name = keymap.key_name;

COMMIT;)SQL_ESC";

/**
 * We check to see if the database has the MBTiles Schema.
 * If not, we execute the schema SQL.
 *
 * @param _source A pointer to a the data source in which we will setup a db.
 */
void setupDB(DataSource& _source) {
    SQLite::Database& db = _source.mbtilesDb();
    bool map = false, grid_key = false, keymap = false, grid_utfgrid = false, images = false,
         metadata = false, geocoder_data = false, tiles = false, grids = false, grid_data = false;

    try {
        SQLite::Statement query(db, "SELECT name FROM sqlite_master WHERE type IN ('table', 'view')");
        while (query.executeStep()) {
            std::string name = query.getColumn(0);
            if (name == "map") map = true;
            else if (name == "grid_key") grid_key = true;
            else if (name == "keymap") keymap = true;
            else if (name == "grid_utfgrid") grid_utfgrid = true;
            else if (name == "images") images = true;
            else if (name == "metadata") metadata = true;
            else if (name == "geocoder_data") geocoder_data = true;
            else if (name == "tiles") tiles = true;
            else if (name == "grids") grids = true;
            else if (name == "grid_data") grid_data = true;
        }
    } catch (std::exception& e) {
        LOGE("Unable to check schema of SQLite MBTiles database: %s", e.what());
    }

    // Return if we have all the tables and views that should exist.
    if (map && grid_key && keymap && grid_utfgrid && images &&
            metadata && geocoder_data && tiles && grids && grid_data) {
        return;
    }

    // Otherwise, we need to execute schema.sql to set up the db with the right schema.
    try {
        // Execute schema.
        db.exec(SCHEMA);

        // Fill in metadata table.
        // https://github.com/pnorman/mbtiles-spec/blob/2.0/2.0/spec.md#content
        // https://github.com/mapbox/mbtiles-spec/pull/46
        SQLite::Statement stmt(db, "REPLACE INTO metadata (name, value) VALUES (?, ?);");

        // name, type, version, description, format, compression
        stmt.bind(1, "name");
        stmt.bind(2, _source.name());
        stmt.exec();
        stmt.reset();

        stmt.bind(1, "type");
        stmt.bind(2, "baselayer");
        stmt.exec();
        stmt.reset();

        stmt.bind(1, "version");
        stmt.bind(2, 1);
        stmt.exec();
        stmt.reset();

        stmt.bind(1, "description");
        stmt.bind(2, "MBTiles tile container created by Tangram ES.");
        stmt.exec();
        stmt.reset();

        stmt.bind(1, "format");
        stmt.bind(2, _source.mimeType());
        stmt.exec();
        stmt.reset();

        // Compression not yet implemented.
        // http://www.iana.org/assignments/http-parameters/http-parameters.xhtml#content-coding
        // identity means no compression
        stmt.bind(1, "compression");
        stmt.bind(2, "identity");
        stmt.exec();

    } catch (std::exception& e) {
        LOGE("Unable to setup SQLite MBTiles database: %s", e.what());
    }

}

}
}
