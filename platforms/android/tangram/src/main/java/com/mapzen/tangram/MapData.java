package com.mapzen.tangram;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

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

    final String name;

    private MapController mapController;
   long pointer = 0;

    /**
     * For package-internal use only; create a new {@code MapData}
     * @param name The name of the associated data source
     * @param pointer The markerId to the native data source, encoded as a long
     * @param map The {@code MapController} associated with this data source
     */
    MapData(final String name, final long pointer, @NonNull final MapController map) {
        this.name = name;
        this.pointer = pointer;
        this.mapController = map;
    }

    /**
     * Add a geometry feature to this data collection
     * @param geometry The feature to add
     */
    protected void addFeature(@NonNull final Geometry geometry) {
        final MapController map = mapController;
        if (map == null) {
            return;
        }

        synchronized (map) {
            nativeAddFeature(pointer,
                    geometry.getCoordinateArray(),
                    geometry.getRingArray(),
                    geometry.getPropertyArray());
        }
    }
    /**
     * Get the name of this {@code MapData}.
     * @return The name.
     */
    public String name() {
        return name;
    }

    /**
     * Remove this {@code MapData} from the map it is currently associated with. Using this object
     * after {@code remove} is called will cause an exception to be thrown. {@code remove} is called
     * on every {@code MapData} associated with a map when its {@code MapController} is destroyed.
     */
    public void remove() {
        final MapController map = mapController;
        if (map == null) {
            return;
        }
        map.removeDataLayer(this);

        mapController = null;
        pointer = 0;
    }

    /**
     * Add a point feature to this collection.
     * @param point The coordinates of the feature.
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     * @return This object, for chaining.
     */
    @NonNull
    public MapData addPoint(@NonNull final LngLat point, @Nullable final Map<String, String> properties) {
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
    @NonNull
    public MapData addPolyline(@NonNull final List<LngLat> polyline, @Nullable final Map<String, String> properties) {
        addFeature(new Polyline(polyline, properties));
        return this;
    }

    /**
     * Add a polygon feature to this collection.
     * @param polygon A list of rings describing the shape of the feature. Each
     * ring is a list of coordinates in which the first point is the same as the last point. The
     * first ring is taken as the "exterior" of the polygon and rings with opposite winding are
     * considered "holes".
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     * @return This object, for chaining.
     */
    @NonNull
    public MapData addPolygon(@NonNull final List<List<LngLat>> polygon, @Nullable final Map<String, String> properties) {
        addFeature(new Polygon(polygon, properties));
        return this;
    }

    /**
     * Add features described in a GeoJSON string to this collection.
     * @param data A string containing a <a href="http://geojson.org/">GeoJSON</a> FeatureCollection
     * @return This object, for chaining.
     */
    @NonNull
    public MapData addGeoJson(final String data) {
        final MapController map = mapController;
        if (map == null) {
            return this;
        }
        synchronized (map) {
            nativeAddGeoJson(pointer, data);
        }
        return this;
    }

    /**
     * Remove all features from this collection.
     * @return This object, for chaining.
     */
    @NonNull
    public MapData clear() {
        final MapController map = mapController;
        if (map == null) {
            return this;
        }
        map.clearTileSource(pointer);
        return this;
    }

    private native void nativeAddFeature(long sourcePtr, double[] coordinates, int[] rings, String[] properties);
    private native void nativeAddGeoJson(long sourcePtr, String geoJson);
}
