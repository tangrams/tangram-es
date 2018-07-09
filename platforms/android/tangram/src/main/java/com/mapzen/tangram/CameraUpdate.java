package com.mapzen.tangram;

public class CameraUpdate {
    CameraPosition position;
    Float zoomBy;
    Float zoomTo;
    Float rotation;
    Float tilt;
    float[] scrollBy;
    LngLat lngLat;

    LngLat[] bounds;
    int padding;

    public CameraUpdate scrollBy(float x, float y) {
        scrollBy = new float[2];
        scrollBy[0] = x;
        scrollBy[1] = y;
        return this;
    }

    public CameraUpdate zoomBy(float dz) {
        zoomBy = new Float(dz);
        return this;
    }

    public CameraUpdate zoomIn() {
        zoomBy = new Float(1);
        return this;
    }

    public CameraUpdate zoomOut() {
        zoomBy = new Float(-1);
        return this;
    }

    public CameraUpdate zoomTo(float zoom) {
        zoomTo = new Float(zoom);
        return this;
    }

    public CameraUpdate setRotation(float radians) {
        rotation = new Float(radians);
        return this;
    }

    public CameraUpdate setTilt(float radians) {
        tilt = new Float(radians);
        return this;
    }

    public static CameraUpdate newCameraPosition(CameraPosition camera) {
        CameraUpdate update = new CameraUpdate();
        update.position = camera;
        return update;
    }

    public static CameraUpdate newLngLatZoom(LngLat lngLat, float zoom) {
        CameraUpdate update = new CameraUpdate();
        update.lngLat = new LngLat(lngLat);
        update.zoomTo = new Float(zoom);
        return update;
    }

    public static CameraUpdate newLatLngBounds(LngLat b1, LngLat b2, int padding) {
        CameraUpdate update = new CameraUpdate();
        update.bounds = new LngLat[] {b1, b2};
        update.padding = padding;
        return update;
    }

}
