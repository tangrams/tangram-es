package com.mapzen.tangram;

import com.mapzen.tangram.geometry.Geometry;

import java.util.List;

import androidx.annotation.NonNull;

/**
 * {@code MapData} is a named collection of drawable map features.
 */
public class MapData {

    private final String name;

    private NativeMapData nativeMapData;

    /**
     * For package-internal use only; create a new {@code MapData}
     * @param name The name of the associated data source
     * @param pointer The markerId to the native data source, encoded as a long
     */
    MapData(@NonNull final String name, final long pointer) {
        this.name = name;
        this.nativeMapData = new NativeMapData(pointer);
    }

    /**
     * Assign a list of features to this data collection. This replaces any previously assigned feature lists or GeoJSON data.
     * @param features The features to assign
     */
    public void setFeatures(@NonNull final List<Geometry> features) {
        nativeMapData.clearClientDataFeatures();
        for (Geometry feature : features) {
            nativeMapData.addClientDataFeature(
                    feature.getCoordinateArray(),
                    feature.getRingArray(),
                    feature.getPropertyArray());
        }
        nativeMapData.generateClientDataTiles();
    }

    /**
     * Assign features described in a GeoJSON string to this collection. This will replace any previously assigned feature lists or GeoJSON data.
     * @param data A string containing a <a href="http://geojson.org/">GeoJSON</a> FeatureCollection
     */
    public void setGeoJson(@NonNull final String data) {
        nativeMapData.clearClientDataFeatures();
        nativeMapData.addClientDataGeoJson(data);
        nativeMapData.generateClientDataTiles();
    }

    /**
     * Get the name of this {@code MapData}.
     * @return The name.
     */
    public String name() {
        return name;
    }

    /**
     * For package-internal use only; this is called on every {@link MapData} associated with a map
     * when its {@link MapController} is destroyed.
     */
    void dispose(NativeMap nativeMap) {
        nativeMapData.remove(nativeMap);
        nativeMapData = null;
    }

    /**
     * Remove all features from this collection.
     */
    public void clear() {
        nativeMapData.clearClientDataFeatures();
        nativeMapData.generateClientDataTiles();
    }
}
