package com.mapzen.tangram;

public class CameraUpdateFactory {
    public static CameraUpdate setPosition(LngLat lngLat) {
        CameraUpdate update = new CameraUpdate();
        update.longitude = lngLat.longitude;
        update.latitude = lngLat.latitude;
        update.set |= CameraUpdate.SET_LNGLAT;
        return update;
    }

    public static CameraUpdate setZoom(float zoom) {
        CameraUpdate update = new CameraUpdate();
        update.zoom = zoom;
        update.set |= CameraUpdate.SET_ZOOM;
        return update;
    }

    public static CameraUpdate setRotation(float radians) {
        CameraUpdate update = new CameraUpdate();
        update.rotation = radians;
        update.set |= CameraUpdate.SET_ROTATION;
        return update;
    }

    public static CameraUpdate setTilt(float radians) {
        CameraUpdate update = new CameraUpdate();
        update.tilt = radians;
        update.set |= CameraUpdate.SET_TILT;
        return update;
    }

    public static CameraUpdate rotateBy(float radians) {
        CameraUpdate update = new CameraUpdate();
        update.rotationBy = radians;
        update.set |= CameraUpdate.SET_ROTATION_BY;
        return update;
    }

    public static CameraUpdate tiltBy(float radians) {
        CameraUpdate update = new CameraUpdate();
        update.tiltBy = radians;
        update.set |= CameraUpdate.SET_TILT_BY;
        return update;
    }

    public static CameraUpdate zoomBy(float dz) {
        CameraUpdate update = new CameraUpdate();
        update.zoomBy = dz;
        update.set |= CameraUpdate.SET_ZOOM_BY;
        return update;
    }

    public static CameraUpdate zoomIn() {
        CameraUpdate update = new CameraUpdate();
        update.zoomBy = 1.f;
        update.set |= CameraUpdate.SET_ZOOM_BY;
        return update;
    }

    public static CameraUpdate zoomOut() {
        CameraUpdate update = new CameraUpdate();
        update.zoomBy = -1.f;
        update.set |= CameraUpdate.SET_ZOOM_BY;
        return update;
    }

    public static CameraUpdate newCameraPosition(CameraPosition position) {
        CameraUpdate update = new CameraUpdate();
        update.set = CameraUpdate.SET_LNGLAT | CameraUpdate.SET_ZOOM | CameraUpdate.SET_ROTATION | CameraUpdate.SET_TILT;
        update.longitude = position.longitude;
        update.latitude = position.latitude;
        update.zoom = position.zoom;
        update.rotation = position.rotation;
        update.tilt = position.tilt;
        return update;
    }
    public static CameraUpdate newLngLatZoom(LngLat lngLat, float zoom) {
        CameraUpdate update = new CameraUpdate();
        update.set = CameraUpdate.SET_LNGLAT | CameraUpdate.SET_ZOOM;
        update.longitude = lngLat.longitude;
        update.latitude = lngLat.latitude;
        update.zoom = zoom;
        return update;
    }

    public static CameraUpdate newLatLngBounds(LngLat b1, LngLat b2, float padding) {
        CameraUpdate update = new CameraUpdate();
        update.set = CameraUpdate.SET_BOUNDS;
        update.boundsLon1 = b1.longitude;
        update.boundsLat1 = b1.latitude;
        update.boundsLon2 = b2.longitude;
        update.boundsLat2 = b2.latitude;
        update.boundsPadding = padding;
        return update;
    }
}
