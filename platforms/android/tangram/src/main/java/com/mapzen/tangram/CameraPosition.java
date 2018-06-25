package com.mapzen.tangram;

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
}
