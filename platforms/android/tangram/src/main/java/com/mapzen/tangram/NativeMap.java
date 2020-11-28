package com.mapzen.tangram;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.graphics.Rect;

class NativeMap {

    NativeMap(MapController mapController, AssetManager assetManager) {
        nativePointer = init(mapController, assetManager);
        if (nativePointer <= 0) {
            throw new RuntimeException("Unable to create a native Map object! There may be insufficient memory available.");
        }
    }

    native long init(MapController mapController, AssetManager assetManager);
    native void dispose();
    native void shutdown();
    native void onLowMemory();
    native int loadScene(String path, String[] updateStrings);
    native int loadSceneAsync(String path, String[] updateStrings);
    native int loadSceneYaml(String yaml, String resourceRoot, String[] updateStrings);
    native int loadSceneYamlAsync(String yaml, String resourceRoot, String[] updateStrings);

    native void setupGL();
    native void resize(int width, int height);
    native int update(float dt);
    native void render();
    native void captureSnapshot(int[] buffer);

    native void getCameraPosition(CameraPosition cameraPositionOut);
    native void updateCameraPosition(int set, double lon, double lat, float zoom, float zoomBy,
                                                                float rotation, float rotateBy, float tilt, float tiltBy,
                                                                double b1lon, double b1lat, double b2lon, double b2lat, Rect padding,
                                                                float duration, int ease);
    native void flyTo(double lon, double lat, float zoom, float duration, float speed);
    native void getEnclosingCameraPosition(LngLat lngLatSE, LngLat lngLatNW, Rect padding, CameraPosition cameraPositionOut);
    native void cancelCameraAnimation();
    native boolean screenPositionToLngLat(float x, float y, LngLat lngLatOut);
    native boolean lngLatToScreenPosition(double lng, double lat, PointF screenPositionOut, boolean clipToViewport);
    native void setPixelScale(float scale);
    native void setCameraType(int type);
    native int getCameraType();
    native float getMinZoom();
    native void setMinZoom(float minZoom);
    native float getMaxZoom();
    native void setMaxZoom(float maxZoom);
    native void handleTapGesture(float posX, float posY);
    native void handleDoubleTapGesture(float posX, float posY);
    native void handlePanGesture(float startX, float startY, float endX, float endY);
    native void handleFlingGesture(float posX, float posY, float velocityX, float velocityY);
    native void handlePinchGesture(float posX, float posY, float scale, float velocity);
    native void handleRotateGesture(float posX, float posY, float rotation);
    native void handleShoveGesture(float distance);
    native void setPickRadius(float radius);
    native void pickFeature(float posX, float posY);
    native void pickLabel(float posX, float posY);
    native void pickMarker(float posX, float posY);
    native long markerAdd();
    native boolean markerRemove(long markerID);
    native boolean markerSetStylingFromString(long markerID, String styling);
    native boolean markerSetStylingFromPath(long markerID, String path);
    native boolean markerSetBitmap(long markerID, Bitmap bitmap, float density);
    native boolean markerSetPoint(long markerID, double lng, double lat);
    native boolean markerSetPointEased(long markerID, double lng, double lat, float duration, int ease);
    native boolean markerSetPolyline(long markerID, double[] coordinates, int count);
    native boolean markerSetPolygon(long markerID, double[] coordinates, int[] rings, int count);
    native boolean markerSetVisible(long markerID, boolean visible);
    native boolean markerSetDrawOrder(long markerID, int drawOrder);
    native void markerRemoveAll();
    native void useCachedGlState(boolean use);
    native void setDefaultBackgroundColor(float r, float g, float b);

    native long addClientDataSource(String name, boolean generateCentroid);
    native void removeClientDataSource(long sourcePtr);
    native void addClientDataFeature(long sourcePtr, double[] coordinates, int[] rings, String[] properties);
    native void addClientDataGeoJson(long sourcePtr, String geoJson);
    native void generateClientDataTiles(long sourcePtr);
    native void clearClientDataFeatures(long sourcePtr);

    native void setDebugFlag(int flag, boolean on);

    native void onUrlComplete(long requestHandle, byte[] rawDataBytes, String errorMessage);

    private final long nativePointer;
}
