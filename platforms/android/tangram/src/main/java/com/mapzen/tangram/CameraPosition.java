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

    public void set(CameraPosition camera) {
        longitude = camera.longitude;
        latitude = camera.latitude;
        zoom = camera.zoom;
        rotation = camera.rotation;
        tilt = camera.tilt;
    }

    public LngLat getPosition(@NonNull LngLat lngLat) {
        lngLat.set(longitude, latitude);
        return lngLat;
    }

    public LngLat getPosition() {
        return getPosition(new LngLat());
    }

    public float getZoom() {
        return zoom;
    }

    public float getRotation() {
        return rotation;
    }

    public float getTilt() {
        return tilt;
    }
}
