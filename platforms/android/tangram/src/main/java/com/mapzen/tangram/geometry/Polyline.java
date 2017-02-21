package com.mapzen.tangram.geometry;

import com.mapzen.tangram.LngLat;

import java.util.List;
import java.util.Map;

/**
 * {@code Polyline} is a sequence of LngLat points and its properties.
 */
public class Polyline extends Geometry {

    public Polyline(List<LngLat> polyline, Map<String, String> properties) {
        this.coordinates = new double[polyline.size() * 2];
        int i = 0;
        for (LngLat point : polyline) {
            coordinates[i++] = point.longitude;
            coordinates[i++] = point.latitude;
        }
        if (properties != null) {
            this.properties = getStringMapAsArray(properties);
        }
    }

}
