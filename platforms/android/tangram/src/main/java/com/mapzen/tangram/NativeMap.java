package com.mapzen.tangram;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.graphics.Rect;

class NativeMap {

    NativeMap(MapController mapController, AssetManager assetManager) {
        nativePointer = init(mapController, assetManager);
        if (nativePointer == 0) {
            throw new RuntimeException("Unable to create a native Map object! There may be insufficient memory available.");
        }
    }

    private native long init(MapController mapController, AssetManager assetManager);

    // Declare all methods called by MapController and MapRenderer to be synchronized. These methods
    // potentially have mutable access to shared native memory and can be called from different
    // threads. If these calls interleave, it can lead to states that cause a segmentation fault.
    native synchronized void dispose();
    native synchronized void shutdown();
    native synchronized void onLowMemory();
    native synchronized int loadScene(String path, String[] updateStrings);
    native synchronized int loadSceneAsync(String path, String[] updateStrings);
    native synchronized int loadSceneYaml(String yaml, String resourceRoot, String[] updateStrings);
    native synchronized int loadSceneYamlAsync(String yaml, String resourceRoot, String[] updateStrings);

    native synchronized void setupGL();
    native synchronized void resize(int width, int height);
    native synchronized int render(float dt);
    native synchronized void captureSnapshot(int[] buffer);

    native synchronized void getCameraPosition(CameraPosition cameraPositionOut);
    native synchronized void updateCameraPosition(int set, double lon, double lat, float zoom, float zoomBy,
                                                                float rotation, float rotateBy, float tilt, float tiltBy,
                                                                double b1lon, double b1lat, double b2lon, double b2lat, EdgePadding padding,
                                                                float duration, int ease);
    native synchronized void flyTo(double lon, double lat, float zoom, float duration, float speed);
    native synchronized void getEnclosingCameraPosition(LngLat lngLatSE, LngLat lngLatNW, EdgePadding padding, CameraPosition cameraPositionOut);
    native synchronized void setPadding(EdgePadding padding);
    native synchronized void getPadding(EdgePadding padding);
    native synchronized void cancelCameraAnimation();
    native synchronized boolean screenPositionToLngLat(float x, float y, LngLat lngLatOut);
    native synchronized boolean lngLatToScreenPosition(double lng, double lat, PointF screenPositionOut, boolean clipToViewport);
    native synchronized void setPixelScale(float scale);
    native synchronized void setCameraType(int type);
    native synchronized int getCameraType();
    native synchronized float getMinZoom();
    native synchronized void setMinZoom(float minZoom);
    native synchronized float getMaxZoom();
    native synchronized void setMaxZoom(float maxZoom);
    native synchronized void handleTapGesture(float posX, float posY);
    native synchronized void handleDoubleTapGesture(float posX, float posY);
    native synchronized void handlePanGesture(float startX, float startY, float endX, float endY);
    native synchronized void handleFlingGesture(float posX, float posY, float velocityX, float velocityY);
    native synchronized void handlePinchGesture(float posX, float posY, float scale, float velocity);
    native synchronized void handleRotateGesture(float posX, float posY, float rotation);
    native synchronized void handleShoveGesture(float distance);
    native synchronized void setPickRadius(float radius);
    native synchronized void pickFeature(float posX, float posY);
    native synchronized void pickLabel(float posX, float posY);
    native synchronized void pickMarker(float posX, float posY);
    native synchronized long markerAdd();
    native synchronized boolean markerRemove(long markerID);
    native synchronized boolean markerSetStylingFromString(long markerID, String styling);
    native synchronized boolean markerSetStylingFromPath(long markerID, String path);
    native synchronized boolean markerSetBitmap(long markerID, Bitmap bitmap, float density);
    native synchronized boolean markerSetPoint(long markerID, double lng, double lat);
    native synchronized boolean markerSetPointEased(long markerID, double lng, double lat, float duration, int ease);
    native synchronized boolean markerSetPolyline(long markerID, double[] coordinates, int count);
    native synchronized boolean markerSetPolygon(long markerID, double[] coordinates, int[] rings, int count);
    native synchronized boolean markerSetVisible(long markerID, boolean visible);
    native synchronized boolean markerSetDrawOrder(long markerID, int drawOrder);
    native synchronized void markerRemoveAll();
    native synchronized void useCachedGlState(boolean use);
    native synchronized void setDefaultBackgroundColor(float r, float g, float b);

    native synchronized long addClientDataSource(String name, boolean generateCentroid);
    native synchronized void removeClientDataSource(long sourcePtr);
    native synchronized void addClientDataFeature(long sourcePtr, double[] coordinates, int[] rings, String[] properties);
    native synchronized void addClientDataGeoJson(long sourcePtr, String geoJson);
    native synchronized void generateClientDataTiles(long sourcePtr);
    native synchronized void clearClientDataFeatures(long sourcePtr);
    native synchronized void setClientDataVisible(long sourcePtr, boolean visible);

    native synchronized void setDebugFlag(int flag, boolean on);

    native void onUrlComplete(long requestHandle, byte[] rawDataBytes, String errorMessage);

    private final long nativePointer;
}
