package com.mapzen.tangram.camera;

import android.graphics.Point;

import com.mapzen.tangram.LngLat;
import com.mapzen.tangram.MapController;

public class CameraUpdate {

    CameraUpdate() {}

    public CameraPosition getCameraPosition(MapController map) {
        return null;
    }

    public static class CameraPositionUpdate extends CameraUpdate {

        private CameraPosition cameraPosition;

        public CameraPositionUpdate(CameraPosition cameraPosition) {
            super();
            this.cameraPosition = cameraPosition;
        }

        @Override
        public CameraPosition getCameraPosition(MapController map) {
            return cameraPosition;
        }
    }

    public static class LngLatUpdate extends CameraUpdate {

        private LngLat target;

        public LngLatUpdate(LngLat target) {
            this.target = target;
        }

        @Override
        public CameraPosition getCameraPosition(MapController map) {
            // FIXME: convert 'rotation' to 'bearing'
            return new CameraPosition(target, map.getZoom(), map.getTilt(), map.getRotation());
        }
    }

    public static class ZoomUpdate extends CameraUpdate {

        private Point focus;
        private float delta;

        public ZoomUpdate(float delta, Point focus) {
            this.delta = delta;
            this.focus = focus;
        }

        public ZoomUpdate(float delta) {
            this.delta = delta;
        }

        @Override
        public CameraPosition getCameraPosition(MapController map) {
            LngLat target = map.getPosition();
            if (focus != null) {
                LngLat fixed = map.coordinatesAtScreenPosition(focus.x, focus.y);
                double s = 1 - Math.pow(2, -delta);
                target.set(
                        target.longitude + s * (fixed.longitude - target.longitude),
                        target.latitude + s * (fixed.latitude - target.latitude)
                );
            }
            float zoom = map.getZoom() + delta;
            // FIXME: convert 'rotation' to 'bearing'
            return new CameraPosition(target, zoom, map.getTilt(), map.getRotation());
        }
    }

    public static class ZoomToUpdate extends CameraUpdate {

        private float zoom;

        public ZoomToUpdate(float zoom) {
            this.zoom = zoom;
        }

        @Override
        public CameraPosition getCameraPosition(MapController map) {
            // FIXME: convert 'rotation' to 'bearing'
            return new CameraPosition(map.getPosition(), zoom, map.getTilt(), map.getRotation());
        }
    }





}
