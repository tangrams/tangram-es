package com.mapzen.tangram;

public class CameraUpdate {

    CameraPosition position;
    Float zoomBy;
    Float zoomTo;
    Float rotation;
    Float tilt;
    LngLat lngLat;
    LngLat[] bounds;
    int padding;

    public CameraUpdate setPosition(LngLat lngLat) {
        this.lngLat = new LngLat(lngLat);
        return this;
    }

    public CameraUpdate setZoom(float zoom) {
        this.zoomTo = new Float(zoom);
        return this;
    }

    public CameraUpdate setRotation(float radians) {
        this.rotation = new Float(radians);
        return this;
    }

    public CameraUpdate setTilt(float radians) {
        this.tilt = new Float(radians);
        return this;
    }

    public CameraUpdate zoomBy(float dz) {
        this.zoomBy = new Float(dz);
        return this;
    }

    public CameraUpdate zoomIn() {
        this.zoomBy = new Float(1);
        return this;
    }

    public CameraUpdate zoomOut() {
        this.zoomBy = new Float(-1);
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

    public static CameraUpdate newLatLngBounds(LngLat b1, LngLat b2, int paddingMeters) {
        CameraUpdate update = new CameraUpdate();
        update.bounds = new LngLat[] {new LngLat(b1), new LngLat(b2)};
        update.padding = paddingMeters;
        return update;
    }

}
