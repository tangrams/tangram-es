package com.mapzen.tangram.geometry;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapzen.tangram.LngLat;

import java.util.Map;

/**
 * {@code Point} is a single LngLat and its properties.
 */
public class Point extends Geometry {

    public Point(@NonNull final LngLat point, @Nullable final Map<String, String> properties) {
        this.coordinates = new double[2];
        coordinates[0] = point.longitude;
        coordinates[1] = point.latitude;
        if (properties != null) {
            this.properties = getStringMapAsArray(properties);
        }

    }
}
