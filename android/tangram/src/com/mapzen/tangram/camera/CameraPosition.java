package com.mapzen.tangram.camera;

import com.mapzen.tangram.LngLat;

public class CameraPosition {

    public final LngLat target;
    public final float zoom;
    public final float tilt;
    public final float bearing;

    public CameraPosition(LngLat target, float zoom, float tilt, float bearing) {
        this.target = target;
        this.zoom = zoom;
        this.tilt = tilt;
        this.bearing = bearing;
    }

    public static class Builder {

        private LngLat target = new LngLat();
        private float zoom = 0;
        private float tilt = 0;
        private float bearing = 0;

        public Builder() {}

        public Builder(CameraPosition previous) {
            this.target = previous.target;
            this.zoom = previous.zoom;
            this.tilt = previous.tilt;
            this.bearing = previous.bearing;
        }

        public CameraPosition build() {
            return new CameraPosition(target, zoom, tilt, bearing);
        }

        public Builder target(LngLat target) {
            this.target = target;
            return this;
        }

        public Builder zoom(float zoom) {
            this.zoom = zoom;
            return this;
        }

        public Builder tilt(float tilt) {
            this.tilt = tilt;
            return this;
        }

        public Builder bearing(float bearing) {
            this.bearing = bearing;
            return this;
        }



    }

}
