package com.mapzen.tangram;

import com.mapzen.tangram.geometry.Geometry;
import com.mapzen.tangram.geometry.Point;
import com.mapzen.tangram.geometry.Polygon;
import com.mapzen.tangram.geometry.Polyline;

import java.util.List;
import java.util.Map;

/**
 * {@code MapData} is a named collection of drawable map features.
 */
public class MapData {

    String name;
    long pointer = 0;
    MapController map;

    /**
     * For package-internal use only; create a new {@code MapData}
     * @param name The name of the associated data source
     * @param pointer The pointer to the native data source, encoded as a long
     * @param map The {@code MapController} associated with this data source
     */
    MapData(String name, long pointer, MapController map) {
        this.name = name;
        this.pointer = pointer;
        this.map = map;
    }

    /**
     * Add a geometry feature to this data collection
     * @param geometry The feature to add
     */
    protected void addFeature(Geometry geometry) {
        map.addFeature(pointer,
                geometry.getCoordinateArray(),
                geometry.getRingArray(),
                geometry.getPropertyArray());
    }

    /**
     * Causes all feature changes ({@link #addPoint(LngLat, Map<String, String>)}, 
     * {@link #addPolyline(List<LngLat>, Map<String, String>)}, etc.)
     * to not be immediately rendered until {@link #endChangeBlock()} is called.
     * Useful to prevent flickering when clearing and redrawing features.
     */
    public void beginChangeBlock()
    {
	map.beginChangeBlock(pointer);
    }

    /**
     * All feature changes since {@link #beginChangeBlock()} will be placed
     * in the rendered scene.
     */
    public void endChangeBlock()
    {
	map.endChangeBlock(pointer);
    }

    /**
     * Get the name of this {@code MapData}.
     * @return The name.
     */
    public String name() {
        return name;
    }

    /**
     * Remove this {@code MapData} from the map it is currently associated with. After
     * {@code remove} is called, this object can no longer apply changes to the map.
     */
    public void remove() {
        map.removeDataLayer(this);
        pointer = 0;
        map = null;
    }

    /**
     * Add a point feature to this collection.
     * @param point The coordinates of the feature.
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     * @return This object, for chaining.
     */
    public MapData addPoint(LngLat point, Map<String, String> properties) {
        addFeature(new Point(point, properties));
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
        addFeature(new Polyline(polyline, properties));
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
        addFeature(new Polygon(polygon, properties));
        return this;
    }

    /**
     * Add features described in a GeoJSON string to this collection.
     * @param data A string containing a <a href="http://geojson.org/">GeoJSON</a> FeatureCollection
     * @return This object, for chaining.
     */
    public MapData addGeoJson(String data) {
        map.addGeoJson(pointer, data);
        return this;
    }

    /**
     * Remove all features from this collection.
     * @return This object, for chaining.
     */
    public MapData clear() {
        map.clearDataSource(pointer);
        return this;
    }

}
