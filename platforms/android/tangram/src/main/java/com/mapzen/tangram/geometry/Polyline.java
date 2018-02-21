package com.mapzen.tangram.geometry;

import android.support.annotation.Keep;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapzen.tangram.LngLat;

import java.util.List;
import java.util.Map;

/**
 * {@code Polyline} is a sequence of LngLat points and its properties.
 */
@Keep
public class Polyline extends Geometry {

    public Polyline(@NonNull final List<LngLat> polyline, @Nullable final Map<String, String> properties) {
        this.coordinates = new double[polyline.size() * 2];
        int i = 0;
        for (final LngLat point : polyline) {
            coordinates[i++] = point.longitude;
            coordinates[i++] = point.latitude;
        }
        if (properties != null) {
            this.properties = getStringMapAsArray(properties);
        }
    }

}
