package com.mapzen.tangram.geometry;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapzen.tangram.LngLat;

import java.util.Map;

/**
 * {@code Point} is a single LngLat and its properties.
 */
public class Point extends Geometry {

    /**
     * Create a point geometry with properties.
     * @param point The coordinates of the feature.
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     */
    public Point(@NonNull final LngLat point, @Nullable final Map<String, String> properties) {
        this.coordinates = new double[2];
        coordinates[0] = point.longitude;
        coordinates[1] = point.latitude;
        if (properties != null) {
            this.properties = getStringMapAsArray(properties);
        }

    }
}
