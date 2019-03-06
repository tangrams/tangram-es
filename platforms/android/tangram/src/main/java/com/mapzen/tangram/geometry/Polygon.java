package com.mapzen.tangram.geometry;

import android.support.annotation.Keep;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapzen.tangram.LngLat;

import java.util.List;
import java.util.Map;

/**
 * {@code Polygon} is a sequence of rings of LngLat points and its properties.
 */
@Keep
public class Polygon extends Geometry {

    /**
     * Create a polygon geometry with properties.
     * @param polygon A list of rings describing the shape of the feature. Each
     * ring is a list of coordinates in which the first point is the same as the last point. The
     * first ring is taken as the "exterior" of the polygon and rings with opposite winding are
     * considered "holes".
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     */
    public Polygon(@NonNull final List<List<LngLat>> polygon, @Nullable final Map<String, String> properties) {
        this.rings = new int[polygon.size()];
        int i = 0;
        int n_points = 0;
        for (final List<LngLat> ring : polygon) {
            n_points += ring.size();
            rings[i++] = ring.size();
        }
        this.coordinates = new double[2 * n_points];
        int j = 0;
        for (final List<LngLat> ring : polygon) {
            for (final LngLat point : ring) {
                coordinates[j++] = point.longitude;
                coordinates[j++] = point.latitude;
            }
        }
        if (properties != null) {
            this.properties = getStringMapAsArray(properties);
        }
    }

}
