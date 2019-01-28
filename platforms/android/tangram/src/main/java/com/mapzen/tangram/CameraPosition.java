package com.mapzen.tangram;

import android.support.annotation.NonNull;

public class CameraPosition {

    public double longitude; // Degrees longitude
    public double latitude;  // Degrees latitude
    public float zoom;
    public float rotation;
    public float tilt;

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        CameraPosition that = (CameraPosition) o;
        return Double.compare(that.longitude, longitude) == 0 &&
                Double.compare(that.latitude, latitude) == 0 &&
                Float.compare(that.zoom, zoom) == 0 &&
                Float.compare(that.rotation, rotation) == 0 &&
                Float.compare(that.tilt, tilt) == 0;
    }

    /**
     * Set this {@link CameraPosition} from the values in another camera.
     * @param camera The camera to use values from
     */
    public void set(@NonNull CameraPosition camera) {
        longitude = camera.longitude;
        latitude = camera.latitude;
        zoom = camera.zoom;
        rotation = camera.rotation;
        tilt = camera.tilt;
    }

    /**
     * Get the longitude and latitude of the center of the camera view.
     * @param lngLat A {@link LngLat} to hold the coordinates
     * @return The {@link LngLat} containing the center coordinates
     */
    public @NonNull LngLat getPosition(@NonNull LngLat lngLat) {
        lngLat.set(longitude, latitude);
        return lngLat;
    }

    /**
     * Get the longitude and latitude of the center of the camera view.
     * @return The {@link LngLat} containing the center coordinates
     */
    public @NonNull LngLat getPosition() {
        return getPosition(new LngLat());
    }

    /**
     * Get the zoom level of the camera view. Lower zoom values correspond to a higher effective
     * altitude of the camera view.
     * @return The zoom level.
     */
    public float getZoom() {
        return zoom;
    }

    /**
     * Get the rotation of the camera view in radians counter-clockwise from North.
     * @return The rotation in radians.
     */
    public float getRotation() {
        return rotation;
    }

    /**
     * Get the tilt angle of the camera view in radians from straight down.
     * @return The tilt angle in radians.
     */
    public float getTilt() {
        return tilt;
    }
}
