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
    int[] padding;
    int set;

    CameraUpdate() {
        set = SET_CAMERA;
    }
}
