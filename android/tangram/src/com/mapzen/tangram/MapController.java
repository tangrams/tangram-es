package com.mapzen.tangram;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.PointF;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.DisplayMetrics;

import com.mapzen.tangram.TouchInput.Gestures;
import com.squareup.okhttp.Callback;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.Response;

import java.io.IOException;
import java.nio.IntBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import okio.BufferedSource;

/**
 * {@code MapController} is the main class for interacting with a Tangram map.
 */
public class MapController implements Renderer {

    /**
     * Options for interpolating map parameters
     */
    public enum EaseType {
        LINEAR,
        CUBIC,
        QUINT,
        SINE,
    }

    /**
     * Options for changing the appearance of 3D geometry
     */
    public enum CameraType {
        PERSPECTIVE,
        ISOMETRIC,
        FLAT,
    }

    protected static EaseType DEFAULT_EASE_TYPE = EaseType.CUBIC;

    /**
     * Options for enabling debug rendering features
     */
    public enum DebugFlag {
        FREEZE_TILES,
        PROXY_COLORS,
        TILE_BOUNDS,
        TILE_INFOS,
        LABELS,
        TANGRAM_INFOS,
        DRAW_ALL_LABELS,
        TANGRAM_STATS,
    }

    /**
     * Interface for a callback to receive information about features picked from the map
     */
    public interface FeaturePickListener {
        /**
         * Receive information about features found in a call to {@link #pickFeature(float, float)}
         * @param properties A mapping of string keys to string or number values
         * @param positionX The horizontal screen coordinate of the center of the feature
         * @param positionY The vertical screen coordinate of the center of the feature
         */
        void onFeaturePick(Map<String, String> properties, float positionX, float positionY);
    }

    public interface ViewCompleteListener {
        /**
         * Called on the render-thread at the end of whenever the view is fully loaded and
         * no ease- or label-animation is running.
         */
        void onViewComplete();
    }

    /**
     * Callback for {@link #captureFrame(FrameCaptureCallback, boolean) }
     */
    public interface FrameCaptureCallback {
        /**
         * Called on the render-thread when a frame was captured.
         */
        void onCaptured(Bitmap bitmap);
    }

    /**
     * Capture MapView as Bitmap.
     * @param waitForCompleteView Delay the capture until the view is fully loaded and
     *                            no ease- or label-animation is running.
     */
    public void captureFrame(FrameCaptureCallback callback, boolean waitForCompleteView) {
        frameCaptureCallback = callback;
        frameCaptureAwaitCompleteView = waitForCompleteView;
        requestRender();
    }

    private Bitmap capture() {
        int w = mapView.getWidth();
        int h = mapView.getHeight();

        int b[] = new int[w * h];
        int bt[] = new int[w * h];

        nativeCaptureSnapshot(mapPointer, b);

        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                int pix = b[i * w + j];
                int pb = (pix >> 16) & 0xff;
                int pr = (pix << 16) & 0x00ff0000;
                int pix1 = (pix & 0xff00ff00) | pr | pb;
                bt[(h - i - 1) * w + j] = pix1;
            }
        }

        return Bitmap.createBitmap(bt, w, h, Bitmap.Config.ARGB_8888);
    }

    /**
     * Construct a MapController using a custom scene file
     * @param view GLSurfaceView for the map display; input events from this
     * view will be handled by the MapController's TouchInput gesture detector.
     * It also provides the Context in which the map will function; the asset
     * bundle for this activity must contain all the local files that the map
     * will need.
     */
    protected MapController(GLSurfaceView view) {

        // Set up MapView
        mapView = view;
        view.setRenderer(this);
        view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        // Set a default HTTPHandler
        httpHandler = new HttpHandler();

        touchInput = new TouchInput(view.getContext());
        view.setOnTouchListener(touchInput);

        setPanResponder(null);
        setScaleResponder(null);
        setRotateResponder(null);
        setShoveResponder(null);

        touchInput.setSimultaneousDetectionAllowed(Gestures.SHOVE, Gestures.ROTATE, false);
        touchInput.setSimultaneousDetectionAllowed(Gestures.ROTATE, Gestures.SHOVE, false);
        touchInput.setSimultaneousDetectionAllowed(Gestures.SHOVE, Gestures.SCALE, false);
        touchInput.setSimultaneousDetectionAllowed(Gestures.SHOVE, Gestures.PAN, false);
        touchInput.setSimultaneousDetectionAllowed(Gestures.SCALE, Gestures.LONG_PRESS, false);
    }

    /**
     * Initialize native Tangram component. This must be called before any use
     * of the MapController!
     * This function is separated from MapController constructor to allow
     * initialization and loading of the Scene on a background thread.
     */
    void init() {
        // Get configuration info from application
        displayMetrics = mapView.getContext().getResources().getDisplayMetrics();
        assetManager = mapView.getContext().getAssets();

        fontFileParser = new FontFileParser();

        // Parse font file desription
        fontFileParser.parse();

        mapPointer = nativeInit(this, assetManager);
        if (mapPointer <= 0) {
            throw new RuntimeException("Unable to create a native Map object! There may be insufficient memory available.");
        }
    }

    void dispose() {
        // Disposing native resources involves GL calls, so we need to run on the GL thread.
        queueEvent(new Runnable() {
            @Override
            public void run() {
                // Dispose each data sources by first removing it from the HashMap values and then
                // calling remove(), so that we don't improperly modify the HashMap while iterating.
                for (Iterator<MapData> it = clientDataSources.values().iterator(); it.hasNext();) {
                    MapData mapData = it.next();
                    it.remove();
                    mapData.remove();
                }
                nativeDispose(mapPointer);
                mapPointer = 0;
                clientDataSources.clear();
            }
        });
    }

    static MapController getInstance(GLSurfaceView view) {
        return new MapController(view);
    }

    /**
     * Load a new scene file
     * @param path Location of the YAML scene file within the application assets
     */
    public void loadSceneFile(String path) {
        scenePath = path;
        checkPointer(mapPointer);
        nativeLoadScene(mapPointer, path);
        requestRender();
    }

    /**
     * Set the {@link HttpHandler} for retrieving remote map resources; a default-constructed
     * HttpHandler is suitable for most cases, but methods can be extended to modify resource URLs
     * @param handler the HttpHandler to use
     */
    public void setHttpHandler(HttpHandler handler) {
        this.httpHandler = handler;
    }

    /**
     * Set the geographic position of the center of the map view
     * @param position LngLat of the position to set
     */
    public void setPosition(LngLat position) {
        checkPointer(mapPointer);
        nativeSetPosition(mapPointer, position.longitude, position.latitude);
    }

    /**
     * Set the geographic position of the center of the map view with default easing
     * @param position LngLat of the position to set
     * @param duration Time in milliseconds to ease to the given position
     */
    public void setPositionEased(LngLat position, int duration) {
        setPositionEased(position, duration, DEFAULT_EASE_TYPE);
    }

    /**
     * Set the geographic position of the center of the map view with custom easing
     * @param position LngLat of the position to set
     * @param duration Time in milliseconds to ease to the given position
     * @param ease Type of easing to use
     */
    public void setPositionEased(LngLat position, int duration, EaseType ease) {
        float seconds = duration / 1000.f;
        checkPointer(mapPointer);
        nativeSetPositionEased(mapPointer, position.longitude, position.latitude, seconds, ease.ordinal());
    }

    /**
     * Get the geographic position of the center of the map view
     * @return The current map position in a LngLat
     */
    public LngLat getPosition() {
        return getPosition(new LngLat());
    }

    /**
     * Get the geographic position of the center of the map view
     * @param out LngLat to be reused as the output
     * @return LngLat of the center of the map view
     */
    public LngLat getPosition(LngLat out) {
        double[] tmp = { 0, 0 };
        checkPointer(mapPointer);
        nativeGetPosition(mapPointer, tmp);
        return out.set(tmp[0], tmp[1]);
    }

    /**
     * Set the zoom level of the map view
     * @param zoom Zoom level; lower values show more area
     */
    public void setZoom(float zoom) {
        checkPointer(mapPointer);
        nativeSetZoom(mapPointer, zoom);
    }

    /**
     * Set the zoom level of the map view with default easing
     * @param zoom Zoom level; lower values show more area
     * @param duration Time in milliseconds to ease to given zoom
     */
    public void setZoomEased(float zoom, int duration) {
        setZoomEased(zoom, duration, DEFAULT_EASE_TYPE);
    }

    /**
     * Set the zoom level of the map view with custom easing
     * @param zoom Zoom level; lower values show more area
     * @param duration Time in milliseconds to ease to given zoom
     * @param ease Type of easing to use
     */
    public void setZoomEased(float zoom, int duration, EaseType ease) {
        float seconds = duration / 1000.f;
        checkPointer(mapPointer);
        nativeSetZoomEased(mapPointer, zoom, seconds, ease.ordinal());
    }

    /**
     * Get the zoom level of the map view
     * @return Zoom level; lower values show more area
     */
    public float getZoom() {
        checkPointer(mapPointer);
        return nativeGetZoom(mapPointer);
    }

    /**
     * Set the rotation of the view
     * @param rotation Counter-clockwise rotation in radians; 0 corresponds to North pointing up
     */
    public void setRotation(float rotation) {
        checkPointer(mapPointer);
        nativeSetRotation(mapPointer, rotation);
    }

    /**
     * Set the rotation of the view with default easing
     * @param rotation Counter-clockwise rotation in radians; 0 corresponds to North pointing up
     * @param duration Time in milliseconds to ease to the given rotation
     */
    public void setRotationEased(float rotation, int duration) {
        setRotationEased(rotation, duration, DEFAULT_EASE_TYPE);
    }

    /**
     * Set the rotation of the view with custom easing
     * @param rotation Counter-clockwise rotation in radians; 0 corresponds to North pointing up
     * @param duration Time in milliseconds to ease to the given rotation
     * @param ease Type of easing to use
     */
    public void setRotationEased(float rotation, int duration, EaseType ease) {
        float seconds = duration / 1000.f;
        checkPointer(mapPointer);
        nativeSetRotationEased(mapPointer, rotation, seconds, ease.ordinal());
    }

    /**
     * Get the rotation of the view
     * @return Counter-clockwise rotation in radians; 0 corresponds to North pointing up
     */
    public float getRotation() {
        checkPointer(mapPointer);
        return nativeGetRotation(mapPointer);
    }

    /**
     * Set the tilt angle of the view
     * @param tilt Tilt angle in radians; 0 corresponds to straight down
     */
    public void setTilt(float tilt) {
        checkPointer(mapPointer);
        nativeSetTilt(mapPointer, tilt);
    }

    /**
     * Set the tilt angle of the view with default easing
     * @param tilt Tilt angle in radians; 0 corresponds to straight down
     * @param duration Time in milliseconds to ease to the given tilt
     */
    public void setTiltEased(float tilt, int duration) {
        setTiltEased(tilt, duration, DEFAULT_EASE_TYPE);
    }

    /**
     * Set the tilt angle of the view with custom easing
     * @param tilt Tilt angle in radians; 0 corresponds to straight down
     * @param duration Time in milliseconds to ease to the given tilt
     * @param ease Type of easing to use
     */
    public void setTiltEased(float tilt, int duration, EaseType ease) {
        float seconds = duration / 1000.f;
        checkPointer(mapPointer);
        nativeSetTiltEased(mapPointer, tilt, seconds, ease.ordinal());
    }

    /**
     * Get the tilt angle of the view
     * @return Tilt angle in radians; 0 corresponds to straight down
     */
    public float getTilt() {
        checkPointer(mapPointer);
        return nativeGetTilt(mapPointer);
    }

    /**
     * Set the camera type for the map view
     * @param type A {@code CameraType}
     */
    public void setCameraType(CameraType type) {
        checkPointer(mapPointer);
        nativeSetCameraType(mapPointer, type.ordinal());
    }

    /**
     * Get the camera type currently in use for the map view
     * @return A {@code CameraType}
     */
    public CameraType getCameraType() {
        checkPointer(mapPointer);
        return CameraType.values()[nativeGetCameraType(mapPointer)];
    }

    /**
     * Find the geographic coordinates corresponding to the given position on screen
     * @param screenPosition Position in pixels from the top-left corner of the map area
     * @return LngLat corresponding to the given point, or null if the screen position
     * does not intersect a geographic location (this can happen at high tilt angles).
     */
    public LngLat screenPositionToLngLat(PointF screenPosition) {
        double[] tmp = { screenPosition.x, screenPosition.y };
        checkPointer(mapPointer);
        if (nativeScreenPositionToLngLat(mapPointer, tmp)) {
            return new LngLat(tmp[0], tmp[1]);
        }
        return null;
    }

    /**
     * Find the position on screen corresponding to the given geographic coordinates
     * @param lngLat Geographic coordinates
     * @return Position in pixels from the top-left corner of the map area (the point
     * may not lie within the viewable screen area)
     */
    public PointF lngLatToScreenPosition(LngLat lngLat) {
        double[] tmp = { lngLat.longitude, lngLat.latitude };
        checkPointer(mapPointer);
        nativeLngLatToScreenPosition(mapPointer, tmp);
        return new PointF((float)tmp[0], (float)tmp[1]);
    }

    /**
     * Construct a collection of drawable map features.
     * @param name The name of the data collection. Once added to a map, features from this
     * {@code MapData} will be available from a data source with this name, just like a data source
     * specified in a scene file. You cannot create more than one data source with the same name.
     * If you call {@code addDataLayer} with the same name more than once, the same {@code MapData}
     * object will be returned.
     */
    public MapData addDataLayer(String name) {
        MapData mapData = clientDataSources.get(name);
        if (mapData != null) {
            return mapData;
        }
        checkPointer(mapPointer);
        long pointer = nativeAddDataSource(mapPointer, name);
        if (pointer <= 0) {
            throw new RuntimeException("Unable to create new data source");
        }
        mapData = new MapData(name, pointer, this);
        clientDataSources.put(name, mapData);
        return mapData;
    }

    /**
     * For package-internal use only; remove a {@code MapData} from this map
     * @param mapData The {@code MapData} to remove
     */
    void removeDataLayer(MapData mapData) {
        clientDataSources.remove(mapData.name);
        checkPointer(mapPointer);
        checkPointer(mapData.pointer);
        nativeRemoveDataSource(mapPointer, mapData.pointer);
    }

    /**
     * Manually trigger a re-draw of the map view
     *
     * Typically this does not need to be called from outside Tangram, see {@link #setRenderMode(int)}.
     */
    public void requestRender() {
        mapView.requestRender();
    }

    /**
     * Set whether the map view re-draws continuously
     *
     * Typically this does not need to be called from outside Tangram. The map automatically re-renders when the view
     * changes or when any animation in the map requires rendering.
     * @param renderMode Either 1, to render continuously, or 0, to render only when needed.
     */
    public void setRenderMode(int renderMode) {
        mapView.setRenderMode(renderMode);
    }

    /**
     * Set a responder for tap gestures
     * @param responder TapResponder to call
     */
    public void setTapResponder(final TouchInput.TapResponder responder) {
        touchInput.setTapResponder(new TouchInput.TapResponder() {
            @Override
            public boolean onSingleTapUp(float x, float y) {
                return responder != null && responder.onSingleTapUp(x, y);
            }

            @Override
            public boolean onSingleTapConfirmed(float x, float y) {
                return responder != null && responder.onSingleTapConfirmed(x, y);
            }
        });
    }

    /**
     * Set a responder for double-tap gestures
     * @param responder DoubleTapResponder to call
     */
    public void setDoubleTapResponder(final TouchInput.DoubleTapResponder responder) {
        touchInput.setDoubleTapResponder(new TouchInput.DoubleTapResponder() {
            @Override
            public boolean onDoubleTap(float x, float y) {
                return responder != null && responder.onDoubleTap(x, y);
            }
        });
    }

    /**
     * Set a responder for long press gestures
     * @param responder LongPressResponder to call
     */
    public void setLongPressResponder(final TouchInput.LongPressResponder responder) {
        touchInput.setLongPressResponder(new TouchInput.LongPressResponder() {
            @Override
            public void onLongPress(float x, float y) {
                if (responder != null) {
                    responder.onLongPress(x, y);
                }
            }
        });
    }

    /**
     * Set a responder for pan gestures
     * @param responder PanResponder to call; if onPan returns true, normal panning behavior will not occur
     */
    public void setPanResponder(final TouchInput.PanResponder responder) {
        touchInput.setPanResponder(new TouchInput.PanResponder() {
            @Override
            public boolean onPan(float startX, float startY, float endX, float endY) {
                if (responder == null || !responder.onPan(startX, startY, endX, endY)) {
                    nativeHandlePanGesture(mapPointer, startX, startY, endX, endY);
                }
                return true;
            }

            @Override
            public boolean onFling(float posX, float posY, float velocityX, float velocityY) {
                if (responder == null || !responder.onFling(posX, posY, velocityX, velocityY)) {
                    nativeHandleFlingGesture(mapPointer, posX, posY, velocityX, velocityY);
                }
                return true;
            }
        });
    }

    /**
     * Set a responder for rotate gestures
     * @param responder RotateResponder to call; if onRotate returns true, normal rotation behavior will not occur
     */
    public void setRotateResponder(final TouchInput.RotateResponder responder) {
        touchInput.setRotateResponder(new TouchInput.RotateResponder() {
            @Override
            public boolean onRotate(float x, float y, float rotation) {
                if (responder == null || !responder.onRotate(x, y, rotation)) {
                    nativeHandleRotateGesture(mapPointer, x, y, rotation);
                }
                return true;
            }
        });
    }

    /**
     * Set a responder for scale gestures
     * @param responder ScaleResponder to call; if onScale returns true, normal scaling behavior will not occur
     */
    public void setScaleResponder(final TouchInput.ScaleResponder responder) {
        touchInput.setScaleResponder(new TouchInput.ScaleResponder() {
            @Override
            public boolean onScale(float x, float y, float scale, float velocity) {
                if (responder == null || !responder.onScale(x, y, scale, velocity)) {
                    nativeHandlePinchGesture(mapPointer, x, y, scale, velocity);
                }
                return true;
            }
        });
    }

    /**
     * Set a responder for shove (vertical two-finger drag) gestures
     * @param responder ShoveResponder to call; if onShove returns true, normal tilting behavior will not occur
     */
    public void setShoveResponder(final TouchInput.ShoveResponder responder) {
        touchInput.setShoveResponder(new TouchInput.ShoveResponder() {
            @Override
            public boolean onShove(float distance) {
                if (responder == null || !responder.onShove(distance)) {
                    nativeHandleShoveGesture(mapPointer, distance);
                }
                return true;
            }
        });
    }

    /**
     * Set whether the gesture {@code second} can be recognized while {@code first} is in progress
     * @param first Initial gesture type
     * @param second Subsequent gesture type
     * @param allowed True if {@code second} should be recognized, else false
     */
    public void setSimultaneousGestureAllowed(Gestures first, Gestures second, boolean allowed) {
        touchInput.setSimultaneousDetectionAllowed(first, second, allowed);
    }

    /**
     * Get whether the gesture {@code second} can be recognized while {@code first} is in progress
     * @param first Initial gesture type
     * @param second Subsequent gesture type
     * @return True if {@code second} will be recognized, else false
     */
    public boolean isSimultaneousGestureAllowed(Gestures first, Gestures second) {
        return touchInput.isSimultaneousDetectionAllowed(first, second);
    }

    /**
     * Set a listener for feature pick events
     * @param listener Listener to call
     */
    public void setFeaturePickListener(FeaturePickListener listener) {
        featurePickListener = listener;
    }

    /**
     * Query the map for labeled features at the given screen coordinates; results will be returned
     * in a callback to the object set by {@link #setFeaturePickListener(FeaturePickListener)}
     * @param posX The horizontal screen coordinate
     * @param posY The vertical screen coordinate
     */
    public void pickFeature(float posX, float posY) {
        if (featurePickListener != null) {
            checkPointer(mapPointer);
            nativePickFeature(mapPointer, posX, posY, featurePickListener);
        }
    }

    /**
     */
    public void setViewCompleteListener(ViewCompleteListener listener) {
        viewCompleteListener = listener;
    }

    /**
     * Enqueue a Runnable to be executed synchronously on the rendering thread
     * @param r Runnable to run
     */
    public void queueEvent(Runnable r) {
        mapView.queueEvent(r);
    }

    /**
     * Make a debugging feature active or inactive
     * @param flag The feature to set
     * @param on True to activate the feature, false to deactivate
     */
    public void setDebugFlag(DebugFlag flag, boolean on) {
        nativeSetDebugFlag(flag.ordinal(), on);
    }

    /**
     * Enqueue a scene component update with its corresponding YAML node value
     * @param componentPath The YAML component path delimited by a '.' (example "scene.animated")
     * @param value A YAML valid string (example "{ property: true }" or "true")
     */
    public void queueSceneUpdate(String componentPath, String value) {
        checkPointer(mapPointer);
        nativeQueueSceneUpdate(mapPointer, componentPath, value);
    }

    /**
     * Apply updates queued by queueSceneUpdate; this empties the current queue of updates
     */
    public void applySceneUpdates() {
        checkPointer(mapPointer);
        nativeApplySceneUpdates(mapPointer);
    }

    /**
     * Set whether the OpenGL state will be cached between subsequent frames. This improves
     * rendering efficiency, but can cause errors if your application code makes OpenGL calls.
     * @param use Whether to use a cached OpenGL state; false by default
     */
    public void useCachedGlState(boolean use) {
        checkPointer(mapPointer);
        nativeUseCachedGlState(mapPointer, use);
    }


    // Package private methods
    // =======================

    void removeDataSource(long sourcePtr) {
        checkPointer(mapPointer);
        checkPointer(sourcePtr);
        nativeRemoveDataSource(mapPointer, sourcePtr);
    }

    void clearDataSource(long sourcePtr) {
        checkPointer(mapPointer);
        checkPointer(sourcePtr);
        nativeClearDataSource(mapPointer, sourcePtr);
    }

    void addFeature(long sourcePtr, double[] coordinates, int[] rings, String[] properties) {
        checkPointer(mapPointer);
        checkPointer(sourcePtr);
        nativeAddFeature(mapPointer, sourcePtr, coordinates, rings, properties);
    }

    void addGeoJson(long sourcePtr, String geoJson) {
        checkPointer(mapPointer);
        checkPointer(sourcePtr);
        nativeAddGeoJson(mapPointer, sourcePtr, geoJson);
    }

    void checkPointer(long ptr) {
        if (ptr <= 0) {
            throw new RuntimeException("Tried to perform an operation on an invalid pointer! This means you may have used an object that has been disposed and is no longer valid.");
        }
    }

    // Native methods
    // ==============

    static {
        System.loadLibrary("tangram");
    }

    private synchronized native long nativeInit(MapController instance, AssetManager assetManager);
    private synchronized native void nativeDispose(long mapPtr);
    private synchronized native void nativeLoadScene(long mapPtr, String path);
    private synchronized native void nativeSetupGL(long mapPtr);
    private synchronized native void nativeResize(long mapPtr, int width, int height);
    private synchronized native boolean nativeUpdate(long mapPtr, float dt);
    private synchronized native void nativeRender(long mapPtr);
    private synchronized native void nativeSetPosition(long mapPtr, double lon, double lat);
    private synchronized native void nativeSetPositionEased(long mapPtr, double lon, double lat, float seconds, int ease);
    private synchronized native void nativeGetPosition(long mapPtr, double[] lonLatOut);
    private synchronized native void nativeSetZoom(long mapPtr, float zoom);
    private synchronized native void nativeSetZoomEased(long mapPtr, float zoom, float seconds, int ease);
    private synchronized native float nativeGetZoom(long mapPtr);
    private synchronized native void nativeSetRotation(long mapPtr, float radians);
    private synchronized native void nativeSetRotationEased(long mapPtr, float radians, float seconds, int ease);
    private synchronized native float nativeGetRotation(long mapPtr);
    private synchronized native void nativeSetTilt(long mapPtr, float radians);
    private synchronized native void nativeSetTiltEased(long mapPtr, float radians, float seconds, int ease);
    private synchronized native float nativeGetTilt(long mapPtr);
    private synchronized native boolean nativeScreenPositionToLngLat(long mapPtr, double[] coordinates);
    private synchronized native boolean nativeLngLatToScreenPosition(long mapPtr, double[] coordinates);
    private synchronized native void nativeSetPixelScale(long mapPtr, float scale);
    private synchronized native void nativeSetCameraType(long mapPtr, int type);
    private synchronized native int nativeGetCameraType(long mapPtr);
    private synchronized native void nativeHandleTapGesture(long mapPtr, float posX, float posY);
    private synchronized native void nativeHandleDoubleTapGesture(long mapPtr, float posX, float posY);
    private synchronized native void nativeHandlePanGesture(long mapPtr, float startX, float startY, float endX, float endY);
    private synchronized native void nativeHandleFlingGesture(long mapPtr, float posX, float posY, float velocityX, float velocityY);
    private synchronized native void nativeHandlePinchGesture(long mapPtr, float posX, float posY, float scale, float velocity);
    private synchronized native void nativeHandleRotateGesture(long mapPtr, float posX, float posY, float rotation);
    private synchronized native void nativeHandleShoveGesture(long mapPtr, float distance);
    private synchronized native void nativeQueueSceneUpdate(long mapPtr, String componentPath, String value);
    private synchronized native void nativeApplySceneUpdates(long mapPtr);
    private synchronized native void nativePickFeature(long mapPtr, float posX, float posY, FeaturePickListener listener);
    private synchronized native void nativeUseCachedGlState(long mapPtr, boolean use);
    private synchronized native void nativeCaptureSnapshot(long mapPtr, int[] buffer);

    private native void nativeOnUrlSuccess(byte[] rawDataBytes, long callbackPtr);
    private native void nativeOnUrlFailure(long callbackPtr);

    synchronized native long nativeAddDataSource(long mapPtr, String name);
    synchronized native void nativeRemoveDataSource(long mapPtr, long sourcePtr);
    synchronized native void nativeClearDataSource(long mapPtr, long sourcePtr);
    synchronized native void nativeAddFeature(long mapPtr, long sourcePtr, double[] coordinates, int[] rings, String[] properties);
    synchronized native void nativeAddGeoJson(long mapPtr, long sourcePtr, String geoJson);

    native void nativeSetDebugFlag(int flag, boolean on);

    // Private members
    // ===============

    private String scenePath;
    private long mapPointer;
    private long time = System.nanoTime();
    private GLSurfaceView mapView;
    private AssetManager assetManager;
    private TouchInput touchInput;
    private FontFileParser fontFileParser;
    private DisplayMetrics displayMetrics = new DisplayMetrics();
    private HttpHandler httpHandler;
    private FeaturePickListener featurePickListener;
    private ViewCompleteListener viewCompleteListener;
    private FrameCaptureCallback frameCaptureCallback;
    private boolean frameCaptureAwaitCompleteView;
    private Map<String, MapData> clientDataSources = new HashMap<>();

    // GLSurfaceView.Renderer methods
    // ==============================

    @Override
    public void onDrawFrame(GL10 gl) {
        long newTime = System.nanoTime();
        float delta = (newTime - time) / 1000000000.0f;
        time = newTime;

        if (mapPointer <= 0) {
            // No native instance is initialized, so stop here. This can happen during Activity
            // shutdown when the map has been disposed but the View hasn't been destroyed yet.
            return;
        }

        boolean viewComplete = nativeUpdate(mapPointer, delta);
        nativeRender(mapPointer);

        if (viewComplete && viewCompleteListener != null) {
            viewCompleteListener.onViewComplete();
        }
        if (frameCaptureCallback != null) {
            if (!frameCaptureAwaitCompleteView || viewComplete) {
                frameCaptureCallback.onCaptured(capture());
                frameCaptureCallback = null;
            }
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        nativeSetPixelScale(mapPointer, displayMetrics.density);
        nativeResize(mapPointer, width, height);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeSetupGL(mapPointer);
    }

    // Networking methods
    // ==================

    void cancelUrlRequest(String url) {
        if (httpHandler == null) {
            return;
        }
        httpHandler.onCancel(url);
    }

    boolean startUrlRequest(String url, final long callbackPtr) throws Exception {
        if (httpHandler == null) {
            return false;
        }
        httpHandler.onRequest(url, new Callback() {
            @Override
            public void onFailure(Request request, IOException e) {
                nativeOnUrlFailure(callbackPtr);
            }

            @Override
            public void onResponse(Response response) throws IOException {
                if (!response.isSuccessful()) {
                    nativeOnUrlFailure(callbackPtr);
                    throw new IOException("Unexpected response code: " + response);
                }
                BufferedSource source = response.body().source();
                byte[] bytes = source.readByteArray();
                nativeOnUrlSuccess(bytes, callbackPtr);
            }
        });
        return true;
    }

    // Font Fetching
    // =============
    String getFontFilePath(String key) {

        return fontFileParser.getFontFile(key);

    }

    String getFontFallbackFilePath(int importance, int weightHint) {

        return fontFileParser.getFontFallback(importance, weightHint);
    }

}
