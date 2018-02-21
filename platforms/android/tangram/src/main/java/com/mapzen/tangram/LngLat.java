package com.mapzen.tangram;

import android.support.annotation.Keep;
import android.support.annotation.NonNull;

/**
 * {@code LngLat} represents a geographic coordinate with longitude and latitude.
 */
@Keep
public class LngLat {

    public double longitude; // Degrees longitude
    public double latitude;  // Degrees latitude

    public LngLat() {
        this(0, 0);
    }

    public LngLat(@NonNull final LngLat other) {
        set(other);
    }

    public LngLat(final double lng, final double lat) {
        set(lng, lat);
    }

    @NonNull
    public LngLat set(final double lng, final double lat) {
        longitude = lng;
        latitude = lat;
        return this;
    }

    @NonNull
    public LngLat set(@NonNull final LngLat other) {
        set(other.longitude, other.latitude);
        return this;
    }

    public boolean equals(final Object other) {
        if (other instanceof LngLat) {
            return longitude == ((LngLat) other).longitude
                && latitude == ((LngLat) other).latitude;
        }
        return false;
    }
}
