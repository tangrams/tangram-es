package com.mapzen.tangram;

import android.graphics.Rect;
import android.support.annotation.NonNull;

public class CameraUpdateFactory {
    /**
     * Create an update that moves the center of the camera view to a new {@link LngLat}
     * @param lngLat The new center coordinates
     * @return The update
     */
    @NonNull
    public static CameraUpdate setPosition(@NonNull LngLat lngLat) {
        CameraUpdate update = new CameraUpdate();
        update.longitude = lngLat.longitude;
        update.latitude = lngLat.latitude;
        update.set |= CameraUpdate.SET_LNGLAT;
        return update;
    }

    /**
     * Create an update that moves the camera view to a new zoom level. Lower zoom values correspond
     * to a higher effective altitude of the camera view.
     * @param zoom The new zoom level
     * @return The update
     */
    @NonNull
    public static CameraUpdate setZoom(float zoom) {
        CameraUpdate update = new CameraUpdate();
        update.zoom = zoom;
        update.set |= CameraUpdate.SET_ZOOM;
        return update;
    }

    /**
     * Create an update that sets the rotations of the camera view around the center point.
     * @param radians The new rotation in radians counter-clockwise from North
     * @return The update
     */
    @NonNull
    public static CameraUpdate setRotation(float radians) {
        CameraUpdate update = new CameraUpdate();
        update.rotation = radians;
        update.set |= CameraUpdate.SET_ROTATION;
        return update;
    }

    /**
     * Create an update that sets the tilt angle of the camera view.
     * @param radians The new tilt angle in radians from straight down
     * @return The update
     */
    @NonNull
    public static CameraUpdate setTilt(float radians) {
        CameraUpdate update = new CameraUpdate();
        update.tilt = radians;
        update.set |= CameraUpdate.SET_TILT;
        return update;
    }

    /**
     * Create an update that rotates the camera view around the center point by a relative amount.
     * @param radians The change in rotation in radians counter-clockwise
     * @return The update
     */
    @NonNull
    public static CameraUpdate rotateBy(float radians) {
        CameraUpdate update = new CameraUpdate();
        update.rotationBy = radians;
        update.set |= CameraUpdate.SET_ROTATION_BY;
        return update;
    }

    /**
     * Create an update that tilts the camera view by a relative amount.
     * @param radians The change in tilt angle in radians
     * @return The update
     */
    @NonNull
    public static CameraUpdate tiltBy(float radians) {
        CameraUpdate update = new CameraUpdate();
        update.tiltBy = radians;
        update.set |= CameraUpdate.SET_TILT_BY;
        return update;
    }

    /**
     * Create an update that changes the camera zoom level by a relative amount.
     * @param dz The change in zoom level
     * @return The update
     */
    @NonNull
    public static CameraUpdate zoomBy(float dz) {
        CameraUpdate update = new CameraUpdate();
        update.zoomBy = dz;
        update.set |= CameraUpdate.SET_ZOOM_BY;
        return update;
    }

    /**
     * Create an update that increases the zoom level of the camera view by 1.
     * @return The update
     */
    @NonNull
    public static CameraUpdate zoomIn() {
        CameraUpdate update = new CameraUpdate();
        update.zoomBy = 1.f;
        update.set |= CameraUpdate.SET_ZOOM_BY;
        return update;
    }

    /**
     * Create an update that decreases the zoom level of the camera view by 1.
     * @return The update
     */
    @NonNull
    public static CameraUpdate zoomOut() {
        CameraUpdate update = new CameraUpdate();
        update.zoomBy = -1.f;
        update.set |= CameraUpdate.SET_ZOOM_BY;
        return update;
    }

    /**
     * Create an update that sets the camera to a new camera position.
     * @param position The new camera position
     * @return The update
     */
    @NonNull
    public static CameraUpdate newCameraPosition(@NonNull CameraPosition position) {
        CameraUpdate update = new CameraUpdate();
        update.set = CameraUpdate.SET_LNGLAT | CameraUpdate.SET_ZOOM | CameraUpdate.SET_ROTATION | CameraUpdate.SET_TILT;
        update.longitude = position.longitude;
        update.latitude = position.latitude;
        update.zoom = position.zoom;
        update.rotation = position.rotation;
        update.tilt = position.tilt;
        return update;
    }

    /**
     * Create an update that sets the center of the camera view and the zoom level.
     * @param lngLat The coordinates of the new center
     * @param zoom The new zoom level
     * @return The update
     */
    @NonNull
    public static CameraUpdate newLngLatZoom(@NonNull LngLat lngLat, float zoom) {
        CameraUpdate update = new CameraUpdate();
        update.set = CameraUpdate.SET_LNGLAT | CameraUpdate.SET_ZOOM;
        update.longitude = lngLat.longitude;
        update.latitude = lngLat.latitude;
        update.zoom = zoom;
        return update;
    }

    /**
     * Create an update that sets the camera view to fit a rectangular area as closely as possible.
     * @param sw The South-West corner of the area
     * @param ne The North-East corner of the area
     * @param padding Pixels of padding to leave around the area
     * @return The update
     */
    @NonNull
    public static CameraUpdate newLngLatBounds(@NonNull LngLat sw, @NonNull LngLat ne, Rect padding) {
        CameraUpdate update = new CameraUpdate();
        update.set = CameraUpdate.SET_BOUNDS;
        update.boundsLon1 = sw.longitude;
        update.boundsLat1 = sw.latitude;
        update.boundsLon2 = ne.longitude;
        update.boundsLat2 = ne.latitude;
        update.padding = new int[]{padding.left, padding.top, padding.right, padding.bottom};
        return update;
    }
}
