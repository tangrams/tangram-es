package com.mapzen.tangram.geometry;

import com.mapzen.tangram.LngLat;

import java.util.List;
import java.util.Map;

/**
 * Geometry is an abstract container of LngLat points and associated properties
 */
public abstract class Geometry {

    List<LngLat> coordinates;
    Map<String, String> properties;
    int[] rings;

    public double[] getCoordinateArray() {
        double[] out = new double[coordinates.size() * 2];
        int i = 0;
        for (LngLat point : coordinates) {
            out[i++] = point.longitude;
            out[i++] = point.latitude;
        }
        return out;
    }

    public int[] getRingArray() {
        return rings;
    }

    public String[] getPropertyArray() {
        String[] out = new String[properties.size() * 2];
        int i = 0;
        for (Map.Entry<String, String> entry : properties.entrySet()) {
            out[i++] = entry.getKey();
            out[i++] = entry.getValue();
        }
        return out;
    }
}
