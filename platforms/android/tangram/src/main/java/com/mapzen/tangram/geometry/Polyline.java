package com.mapzen.tangram.geometry;

import com.mapzen.tangram.LngLat;

import java.util.List;
import java.util.Map;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * {@code Polyline} is a sequence of LngLat points and its properties.
 */
@Keep
public class Polyline extends Geometry {

    public static double[] toCoordinateArray(List<LngLat> polyline) {
        double[] coordinates = new double[polyline.size() * 2];
        int i = 0;
        for (final LngLat point : polyline) {
            coordinates[i++] = point.longitude;
            coordinates[i++] = point.latitude;
        }
        return coordinates;
    }

    /**
     * Create a polyline geometry with properties.
     * @param polyline A list of coordinates that define the line segments of the feature.
     * @param properties The properties of the feature, used for filtering and styling according to
     * the scene file used by the map; may be null.
     */
    public Polyline(@NonNull final List<LngLat> polyline, @Nullable final Map<String, String> properties) {
        this.coordinates = toCoordinateArray(polyline);
        if (properties != null) {
            this.properties = getStringMapAsArray(properties);
        }
    }

}
