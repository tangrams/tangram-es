package com.mapzen.tangram;

import com.mapzen.tangram.geometry.Geometry;

import java.util.List;

import androidx.annotation.NonNull;

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
        checkPointer(pointer);
        final NativeMap nativeMap = mapController.nativeMap;
        nativeMap.clearClientDataFeatures(pointer);
        for (Geometry feature : features) {
            nativeMap.addClientDataFeature(pointer,
                    feature.getCoordinateArray(),
                    feature.getRingArray(),
                    feature.getPropertyArray());
        }
        nativeMap.generateClientDataTiles(pointer);
    }

    /**
     * Assign features described in a GeoJSON string to this collection. This will replace any previously assigned feature lists or GeoJSON data.
     * @param data A string containing a <a href="http://geojson.org/">GeoJSON</a> FeatureCollection
     */
    public void setGeoJson(final String data) {
        checkPointer(pointer);
        final NativeMap nativeMap = mapController.nativeMap;
        nativeMap.clearClientDataFeatures(pointer);
        nativeMap.addClientDataGeoJson(pointer, data);
        nativeMap.generateClientDataTiles(pointer);
    }

    /**
     * Set the visibility of all the features in this {@code MapData}.
     * @param visible If true, the features are drawn on the map. Otherwise they are hidden.
     */
    public void setVisible(boolean visible) {
        checkPointer(pointer);
        mapController.nativeMap.setClientDataVisible(pointer, visible);
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
    }

    void dispose() {
        mapController = null;
        pointer = 0;
    }

    /**
     * Remove all features from this collection.
     */
    public void clear() {
        checkPointer(pointer);
        final NativeMap nativeMap = mapController.nativeMap;
        nativeMap.clearClientDataFeatures(pointer);
        nativeMap.generateClientDataTiles(pointer);
    }

    private void checkPointer(final long ptr) {
        if (ptr == 0) {
            throw new RuntimeException("Tried to perform an operation on an invalid pointer!"
                    + " This means you may have used a MapData that has already been removed.");
        }
    }
}
