package com.mapzen.tangram.geometry;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

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

    @Nullable
    public String[] getPropertyArray() {
        return properties;
    }

    @NonNull
    protected String[] getStringMapAsArray(@NonNull final Map<String, String> properties) {
        final String[] out = new String[properties.size() * 2];
        int i = 0;
        for (final Map.Entry<String, String> entry : properties.entrySet()) {
            out[i++] = entry.getKey();
            out[i++] = entry.getValue();
        }
        return out;
    }
}
