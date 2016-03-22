package com.mapzen.tangram.geometry;

import com.mapzen.tangram.LngLat;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * A single LngLat and its properties
 */
public class Point extends Geometry {

    public Point(LngLat point, Map<String, String> properties) {
        this.coordinates = new ArrayList<>(1);
        coordinates.add(new LngLat(point));
        if (properties != null) {
            this.properties = new HashMap<>(properties);
        }

    }
}
