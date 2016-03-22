package com.mapzen.tangram.geometry;

import com.mapzen.tangram.LngLat;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A sequence of rings of LngLat points and its properties
 */
public class Polygon extends Geometry {

    public Polygon(List<List<LngLat>> polygon, Map<String, String> properties) {
        this.coordinates = new ArrayList<>();
        this.rings = new int[polygon.size()];
        int i = 0;
        for (List<LngLat> ring : polygon) {
            coordinates.addAll(ring);
            rings[i++] = ring.size();
        }
        if (properties != null) {
            this.properties = new HashMap<>(properties);
        }
    }

}
