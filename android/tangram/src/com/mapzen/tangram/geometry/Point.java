package com.mapzen.tangram.geometry;

import com.mapzen.tangram.LngLat;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * {@code Point} is a single LngLat and its properties.
 *
 * Users of Tangram do not need to use this class.
 */
public class Point extends Geometry {

    public Point(LngLat point, Map<String, String> properties) {
        this.coordinates = new double[2];
        coordinates[0] = point.longitude;
        coordinates[1] = point.latitude;
        if (properties != null) {
            this.properties = getStringMapAsArray(properties);
        }

    }
}
