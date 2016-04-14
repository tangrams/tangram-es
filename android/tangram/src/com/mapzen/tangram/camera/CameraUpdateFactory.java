package com.mapzen.tangram.camera;

import android.graphics.Point;

import com.mapzen.tangram.LngLat;

public class CameraUpdateFactory {

    public static CameraUpdate newCameraPosition(CameraPosition cameraPosition) {
        return new CameraUpdate.CameraPositionUpdate(cameraPosition);
    }

    public static CameraUpdate newLngLat(LngLat lnglat) {
        return new CameraUpdate.LngLatUpdate(lnglat);
    }

    public static CameraUpdate zoomBy(float amount, Point focus) {
        return new CameraUpdate.ZoomUpdate(amount, focus);
    }

    public static CameraUpdate zoomBy(float amount) {
        return new CameraUpdate.ZoomUpdate(amount);
    }

    public static CameraUpdate zoomIn() {
        return new CameraUpdate.ZoomUpdate(1);
    }

    public static CameraUpdate zoomOut() {
        return new CameraUpdate.ZoomUpdate(-1);
    }

    public static CameraUpdate zoomTo(float amount) {
        return new CameraUpdate.ZoomToUpdate(amount);
    }

}
