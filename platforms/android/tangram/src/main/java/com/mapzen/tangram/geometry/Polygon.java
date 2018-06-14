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
