package com.mapzen.tangram;

import android.graphics.PointF;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * Result of a marker selection query.
 */
@Keep
public class MarkerPickResult {

    private final Marker marker;
    private final LngLat coordinates;
    private final PointF screenPosition;

    MarkerPickResult(final Marker marker, final double longitude, final double latitude, final float screenX, final float screenY) {
        this.marker = marker;
        this.coordinates = new LngLat(longitude, latitude);
        this.screenPosition = new PointF(screenX, screenY);
    }

    /**
     * @return Selected marker.
     */
    public Marker getMarker() {
        return marker;
    }

    /**
     * @return Geographic coordinates of the selected marker.
     */
    @NonNull
    public LngLat getCoordinates() {
        return coordinates;
    }

    /**
     * @return Screen position of the query that produced this result.
     */
    @NonNull
    public PointF getScreenPosition() {
        return screenPosition;
    }

}
