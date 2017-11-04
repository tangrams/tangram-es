package com.mapzen.tangram.geometry;

import android.support.annotation.Keep;

import com.mapzen.tangram.LngLat;

import java.util.List;
import java.util.Map;

/**
 * {@code Polyline} is a sequence of LngLat points and its properties.
 */
@Keep
public class Polyline extends Geometry {

    public static double[] toCoordinateArray(List<LngLat> polyline) {
        double[] coordinates = new double[polyline.size() * 2];
        int i = 0;
        for (LngLat point : polyline) {
            coordinates[i++] = point.longitude;
            coordinates[i++] = point.latitude;
        }
        return coordinates;
    }

    public Polyline(List<LngLat> polyline, Map<String, String> properties) {
        this.coordinates = toCoordinateArray(polyline);
        if (properties != null) {
            this.properties = getStringMapAsArray(properties);
        }
    }

}
