package com.mapzen.tangram;

import com.mapzen.tangram.geometry.Geometry;
import com.mapzen.tangram.geometry.Point;
import com.mapzen.tangram.geometry.Polygon;
import com.mapzen.tangram.geometry.Polyline;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * {@code MapData} is a named collection of drawable map features.
 */
public class MapData {

    String name;
    long pointer = 0;
    MapController map;
    List<String> geojson = new ArrayList<>();
    List<Geometry> features = new ArrayList<>();

    /**
     * Construct a collection of drawable map features.
     * @param name The name of the data collection. Once added to a map, features from this
     * {@code MapData} will be available from a data source with this name, just like a data source
     * specified in a scene file.
     */
    public MapData(String name) {
        this.name = name;
    }

    /**
     * Get the name of this {@code MapData}.
     * @return The name.
     */
    public String name() {
        return name;
    }

    /**
     * Add the features from this {@code MapData} to a map.
     * <p>
     * This {@code MapData} will be associated with the given map until {@link #removeFromMap()}
     * is called or until this method is called again.
     * @param map The {@code MapController} managing the destination map.
     */
    public void addToMap(MapController map) {
        if (map == null) {
            throw new RuntimeException("MapData cannot be added to a null MapController");
        }
        if (this.map != null) {
            removeFromMap();
        }
        this.map = map;
        this.pointer = map.nativeAddDataSource(name);
        syncWithMap();
    }

    /**
     * Remove this {@code MapData} from the map it is currently associated with. This must not be
     * called when this {@code MapData} is not associated with a map.
     */
    public void removeFromMap() {
        if (map == null) {
            throw new RuntimeException("There is no associated map for this MapData");
        }
        map.nativeRemoveDataSource(pointer);
        pointer = 0;
        map = null;
    }

    /**
     * Synchronize the features in this {@code MapData} with the map.
     * <p>
     * This method must be called after features are added or removed to update their
     * appearance in the map. This method must not be called when this {@code MapData} is not
     * associated with a map.
     */
    public void syncWithMap() {
        if (map == null) {
            throw new RuntimeException("There is no associated map for this MapData");
        }
        map.nativeClearDataSource(pointer);
        for (String data : geojson) {
            map.nativeAddGeoJson(pointer, data);
        }
        for (Geometry geometry : features) {
            map.nativeAddFeature(pointer,
                    geometry.getCoordinateArray(),
                    geometry.getRingArray(),
                    geometry.getPropertyArray());
        }
    }

    /**
     * Add a point feature to this collection.
     * @param point The coordinates of the feature.
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     * @return This object, for chaining.
     */
    public MapData addPoint(LngLat point, Map<String, String> properties) {
        features.add(new Point(point, properties));
        return this;
    }

    /**
     * Add a polyline feature to this collection.
     * @param polyline A list of coordinates that define the line segments of the feature.
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     * @return This object, for chaining.
     */
    public MapData addPolyline(List<LngLat> polyline, Map<String, String> properties) {
        features.add(new Polyline(polyline, properties));
        return this;
    }

    /**
     * Add a polygon feature to this collection.
     * @param polygon A list of rings describing the shape of the feature. Each
     * ring is a list of coordinates. The first ring is taken as the "exterior" of the polygon and
     * rings with opposite winding are considered "holes".
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     * @return This object, for chaining.
     */
    public MapData addPolygon(List<List<LngLat>> polygon, Map<String, String> properties) {
        features.add(new Polygon(polygon, properties));
        return this;
    }

    /**
     * Add features described in a GeoJSON string to this collection.
     * @param data A string containing a <a href="http://geojson.org/">GeoJSON</a> FeatureCollection
     * @return This object, for chaining.
     */
    public MapData addGeoJson(String data) {
        geojson.add(data);
        return this;
    }

    /**
     * Remove all features from this collection.
     * @return This object, for chaining.
     */
    public MapData clear() {
        geojson.clear();
        features.clear();
        return this;
    }

}
