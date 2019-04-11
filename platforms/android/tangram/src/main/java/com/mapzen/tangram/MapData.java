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
    long pointer;

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
     * Assign a list of features to this data collection. This replaces any previously assigned feature lists or GeoJSON data.
     * @param features The features to assign
     */
    public void setFeatures(@NonNull final List<Geometry> features) {
        final MapController map = mapController;
        if (map == null) {
            return;
        }
        synchronized (this) {
            map.clearTileSource(pointer);
            for (Geometry feature : features) {
                nativeAddFeature(pointer,
                        feature.getCoordinateArray(),
                        feature.getRingArray(),
                        feature.getPropertyArray());
            }
            nativeGenerateTiles(pointer);
        }
    }

    /**
     * Assign features described in a GeoJSON string to this collection. This will replace any previously assigned feature lists or GeoJSON data.
     * @param data A string containing a <a href="http://geojson.org/">GeoJSON</a> FeatureCollection
     */
    public void setGeoJson(final String data) {
        final MapController map = mapController;
        if (map != null) {
            synchronized (this) {
                map.clearTileSource(pointer);
                nativeAddGeoJson(pointer, data);
                nativeGenerateTiles(pointer);
            }
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
     * Remove all features from this collection.
     */
    public void clear() {
        final MapController map = mapController;
        if (map != null) {
            map.clearTileSource(pointer);
        }
    }

    private native void nativeAddFeature(long sourcePtr, double[] coordinates, int[] rings, String[] properties);
    private native void nativeAddGeoJson(long sourcePtr, String geoJson);
    private native void nativeGenerateTiles(long sourcePtr);
}
