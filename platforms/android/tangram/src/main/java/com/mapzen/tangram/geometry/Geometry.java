package com.mapzen.tangram.geometry;

import java.util.Map;

/**
 * {@code Geometry} is an abstract container of LngLat points and associated properties.
 */
public abstract class Geometry {

    protected double[] coordinates;
    protected int[] rings;
    protected String[] properties;

    public double[] getCoordinateArray() {
        return coordinates;
    }

    public int[] getRingArray() {
        return rings;
    }

    public String[] getPropertyArray() {
        return properties;
    }

    protected String[] getStringMapAsArray(Map<String, String> properties) {
        String[] out = new String[properties.size() * 2];
        int i = 0;
        for (Map.Entry<String, String> entry : properties.entrySet()) {
            out[i++] = entry.getKey();
            out[i++] = entry.getValue();
        }
        return out;
    }
}
