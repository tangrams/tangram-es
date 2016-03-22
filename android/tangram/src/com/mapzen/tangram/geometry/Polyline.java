package com.mapzen.tangram.geometry;

import com.mapzen.tangram.LngLat;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A sequence of LngLat points and its properties
 */
public class Polyline extends Geometry {

    public Polyline(List<LngLat> polyline, Map<String, String> properties) {
        this.coordinates = new ArrayList<>(polyline);
        if (properties != null) {
            this.properties = new HashMap<>(properties);
        }
    }
    
}
