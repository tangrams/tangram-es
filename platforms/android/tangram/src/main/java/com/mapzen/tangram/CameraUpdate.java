package com.mapzen.tangram;

public class CameraUpdate {


    final static int SET_LNGLAT = 1 << 0;
    final static int SET_ZOOM = 1 << 1;
    final static int SET_ZOOM_BY = 1 << 2;
    final static int SET_ROTATION = 1 << 3;
    final static int SET_ROTATION_BY = 1 << 4;
    final static int SET_TILT = 1 << 5;
    final static int SET_TILT_BY = 1 << 6;
    final static int SET_BOUNDS = 1 << 7;
    final static int SET_CAMERA = 1 << 8;

    float zoomBy;
    float zoom;
    float rotation;
    float rotationBy;
    float tilt;
    float tiltBy;
    double longitude;
    double latitude;
    double boundsLon1;
    double boundsLat1;
    double boundsLon2;
    double boundsLat2;
    float boundsPadding;
    int set;

    public CameraUpdate setPosition(LngLat lngLat) {
        this.longitude = lngLat.longitude;
        this.latitude = lngLat.latitude;
        this.set |= SET_LNGLAT;
        return this;
    }

    public CameraUpdate setZoom(float zoom) {
        this.zoom = zoom;
        this.set |= SET_ZOOM;
        return this;
    }

    public CameraUpdate setRotation(float radians) {
        this.rotation = radians;
        this.set |= SET_ROTATION;
        return this;
    }

    public CameraUpdate setTilt(float radians) {
        this.tilt = radians;
        this.set |= SET_TILT;
        return this;
    }

    public CameraUpdate rotateBy(float radians) {
        this.rotationBy = radians;
        this.set |= SET_ROTATION_BY;
        return this;
    }

    public CameraUpdate tiltBy(float radians) {
        this.tiltBy = radians;
        this.set |= SET_TILT_BY;
        return this;
    }

    public CameraUpdate zoomBy(float dz) {
        this.zoomBy = dz;
        this.set |= SET_ZOOM_BY;
        return this;
    }

    public CameraUpdate zoomIn() {
        this.zoomBy = 1.f;
        this.set |= SET_ZOOM_BY;
        return this;
    }

    public CameraUpdate zoomOut() {
        this.zoomBy = -1.f;
        this.set |= SET_ZOOM_BY;
        return this;
    }

    public CameraUpdate() {
        set = SET_CAMERA;
    }

    public static CameraUpdate newCameraPosition(CameraPosition position) {
        CameraUpdate update = new CameraUpdate();
        update.set = SET_LNGLAT | SET_ZOOM | SET_ROTATION | SET_TILT;
        update.longitude = position.longitude;
        update.latitude = position.latitude;
        update.zoom = position.zoom;
        update.rotation = position.rotation;
        update.tilt = position.tilt;
        return update;
    }
    public static CameraUpdate newLngLatZoom(LngLat lngLat, float zoom) {
        CameraUpdate update = new CameraUpdate();
        update.longitude = lngLat.longitude;
        update.latitude = lngLat.latitude;
        update.zoom = zoom;
        update.set = SET_LNGLAT | SET_ZOOM;
        return update;
    }

    public static CameraUpdate newLatLngBounds(LngLat b1, LngLat b2, float padding) {
        CameraUpdate update = new CameraUpdate();
        update.boundsLon1 = b1.longitude;
        update.boundsLat1 = b1.latitude;
        update.boundsLon2 = b2.longitude;
        update.boundsLat2 = b2.latitude;
        update.boundsPadding = padding;
        update.set = SET_BOUNDS;
        return update;
    }

}
