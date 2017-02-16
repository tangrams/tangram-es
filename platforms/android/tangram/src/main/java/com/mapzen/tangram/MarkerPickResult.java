package com.mapzen.tangram;

/**
 * {@code MarkerPickResult} represents labels that can be selected on the screen
 */

public class MarkerPickResult {

    private Marker marker;
    private LngLat coordinates;

    private MarkerPickResult(Marker marker, double longitude, double latitude) {
        this.marker = marker;
        this.coordinates = new LngLat(longitude, latitude);
    }

    /**
     * @return The marker associated with the selection
     */
    public Marker getMarker() {
        return this.marker;
    }

    /**
     * @return The coordinate of the feature for which this label has been created
     */
    public LngLat getCoordinates() {
        return this.coordinates;
    }

}
