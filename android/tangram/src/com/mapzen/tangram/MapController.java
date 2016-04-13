package com.mapzen.tangram;

import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.DisplayMetrics;

import com.mapzen.tangram.TouchInput.Gestures;
import com.squareup.okhttp.Callback;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.Response;

import java.io.IOException;
import java.util.Map;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import okio.BufferedSource;

public class MapController implements Renderer {

    public enum EaseType {
        LINEAR,
        CUBIC,
        QUINT,
        SINE,
    }

    public enum CameraType {
        PERSPECTIVE,
        ISOMETRIC,
        FLAT,
    }

    public enum DebugFlag {
        FREEZE_TILES,
        PROXY_COLORS,
        TILE_BOUNDS,
        TILE_INFOS,
        LABELS,
        TANGRAM_INFOS,
        ALL_LABELS,
    }

    public interface FeaturePickListener {
        /**
         * Receive information about features found in a call to {@link #pickFeature(float, float)}
         * @param properties A mapping of string keys to string or number values
         * @param positionX The horizontal screen coordinate of the center of the feature
         * @param positionY The vertical screen coordinate of the center of the feature
         */
        void onFeaturePick(Map<String, String> properties, float positionX, float positionY);
    }

    /**
     * Construct a MapController using a custom scene file
     * @param context Context in which the map will function; the asset bundle for this activity
     * must contain all the local files that the map will need
     * @param sceneFilePath Location of the YAML scene file within the assets directory
     */
    MapController(Context context, String sceneFilePath) {

        scenePath = sceneFilePath;

        // Get configuration info from application
        displayMetrics = context.getResources().getDisplayMetrics();
        assetManager = context.getAssets();

        // Load the fonts
        fontFileParser = new FontFileParser();
        fontFileParser.parse("/system/etc/fonts.xml");

        nativeInit(this, assetManager, scenePath);

    }

    /**
     * Set the view in which the map will be drawn
     * @param view GLSurfaceView where the map will be displayed; input events from this view will
     * be handled by the resulting MapController
     */
    void setView(GLSurfaceView view) {
        touchInput = new TouchInput(view.getContext());
        setPanResponder(null);
        setScaleResponder(null);
        setRotateResponder(null);
        setShoveResponder(null);

        touchInput.setSimultaneousDetectionAllowed(Gestures.SHOVE, Gestures.ROTATE, false);
        touchInput.setSimultaneousDetectionAllowed(Gestures.ROTATE, Gestures.SHOVE, false);
        touchInput.setSimultaneousDetectionAllowed(Gestures.SHOVE, Gestures.SCALE, false);
        touchInput.setSimultaneousDetectionAllowed(Gestures.SHOVE, Gestures.PAN, false);
        touchInput.setSimultaneousDetectionAllowed(Gestures.SCALE, Gestures.LONG_PRESS, false);

        // Set up MapView
        mapView = view;
        view.setOnTouchListener(touchInput);
        view.setRenderer(this);
        view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    /**
     * Load a new scene file
     * @param path Location of the YAML scene file within the application assets
     */
    public void loadSceneFile(String path) {
        scenePath = path;
        nativeLoadScene(path);
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
    public void setMapPosition(LngLat position) {
        setMapPosition(position.longitude, position.latitude);
    }

    /**
     * Set the geographic position of the center of the map view
     * @param lng Degrees longitude of the position to set
     * @param lat Degrees latitude of the position to set
     */
    public void setMapPosition(double lng, double lat) {
        nativeSetPosition(lng, lat);
    }

    /**
     * Set the geographic position of the center of the map view
     * @param lng Degrees longitude of the position to set
     * @param lat Degrees latitude of the position to set
     * @param duration Time in seconds to ease to the given position
     */
    public void setMapPosition(double lng, double lat, float duration) {
        nativeSetPositionEased(lng, lat, duration, EaseType.QUINT.ordinal());
    }

    /**
     * Set the geographic position of the center of the map view
     * @param lng Degrees longitude of the position to set
     * @param lat Degrees latitude of the position to set
     * @param duration Time in seconds to ease to the given position
     * @param ease Type of easing to use
     */
    public void setMapPosition(double lng, double lat, float duration, EaseType ease) {
        nativeSetPositionEased(lng, lat, duration, ease.ordinal());
    }

    /**
     * Get the geographic position of the center of the map view
     * @return The current map position in a LngLat
     */
    public LngLat getMapPosition() {
        return getMapPosition(new LngLat());
    }

    /**
     * Get the geographic position of the center of the map view
     * @param out LngLat to be reused as the output
     * @return Degrees longitude and latitude of the current map position, in a two-element array
     */
    public LngLat getMapPosition(LngLat out) {
        double[] tmp = { 0, 0 };
        nativeGetPosition(tmp);
        return out.set(tmp[0], tmp[1]);
    }

    /**
     * Set the zoom level of the map view
     * @param zoom Fractional zoom level
     */
    public void setMapZoom(float zoom) {
        nativeSetZoom(zoom);
    }

    /**
     * Set the zoom level of the map view
     * @param zoom Fractional zoom level
     * @param duration Time in seconds to ease to given zoom
     */
    public void setMapZoom(float zoom, float duration) {
        nativeSetZoomEased(zoom, duration, EaseType.QUINT.ordinal());
    }

    /**
     * Set the zoom level of the map view
     * @param zoom Fractional zoom level
     * @param duration Time in seconds to ease to given zoom
     * @param ease Type of easing to use
     */
    public void setMapZoom(float zoom, float duration, EaseType ease) {
        nativeSetZoomEased(zoom, duration, ease.ordinal());
    }

    /**
     * Get the zoom level of the map view
     * @return Fractional zoom level
     */
    public float getMapZoom() {
        return nativeGetZoom();
    }

    /**
     * Set the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
     * @param radians Rotation in radians
     */
    public void setMapRotation(float radians) {
        nativeSetRotation(radians);
    }

    /**
     * Set the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
     * @param radians Rotation in radians
     * @param duration Time in seconds to ease to the given rotation
     */
    public void setMapRotation(float radians, float duration) {
        nativeSetRotationEased(radians, duration, EaseType.QUINT.ordinal());
    }

    /**
     * Set the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
     * @param radians Rotation in radians
     * @param duration Time in seconds to ease to the given rotation
     * @param ease Type of easing to use
     */
    public void setMapRotation(float radians, float duration, EaseType ease) {
        nativeSetRotationEased(radians, duration, ease.ordinal());
    }

    /**
     * Get the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
     * @return Rotation in radians
     */
    public float getMapRotation() {
        return nativeGetRotation();
    }

    /**
     * Set the tilt angle of the view in radians; 0 corresponds to straight down
     * @param radians Tilt angle in radians
     */
    public void setMapTilt(float radians) {
        nativeSetTilt(radians);
    }

    /**
     * Set the tilt angle of the view in radians; 0 corresponds to straight down
     * @param radians Tilt angle in radians
     * @param duration Time in seconds to ease to the given tilt
     */
    public void setMapTilt(float radians, float duration) {
        nativeSetTiltEased(radians, duration, EaseType.QUINT.ordinal());
    }

    /**
     * Set the tilt angle of the view in radians; 0 corresponds to straight down
     * @param radians Tilt angle in radians
     * @param duration Time in seconds to ease to the given tilt
     * @param ease Type of easing to use
     */
    public void setMapTilt(float radians, float duration, EaseType ease) {
        nativeSetTiltEased(radians, duration, ease.ordinal());
    }

    /**
     * Get the tilt angle of the view in radians; 0 corresponds to straight down
     * @return Tilt angle in radians
     */
    public float getMapTilt() {
        return nativeGetTilt();
    }

    /**
     * Set the camera type for the map view
     * @param type A CameraType
     */
    public void setMapCameraType(CameraType type) {
        nativeSetCameraType(type.ordinal());
        requestRender();
    }

    /**
     * Get the camera type currently in use for the map view
     * @return The current CameraType
     */
    public CameraType getMapCameraType() {
        return CameraType.values()[nativeGetCameraType()];
    }

    /**
     * Find the geographic coordinates corresponding to the given position on screen
     * @param screenX Pixels from the left edge of the screen
     * @param screenY Pixels from the top edge of the screen
     * @return LngLat corresponding to the given point
     */
    public LngLat coordinatesAtScreenPosition(double screenX, double screenY) {
        double[] tmp = { screenX, screenY };
        nativeScreenToWorldCoordinates(tmp);
        return new LngLat(tmp[0], tmp[1]);
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
                    nativeHandlePanGesture(startX, startY, endX, endY);
                }
                return true;
            }

            @Override
            public boolean onFling(float posX, float posY, float velocityX, float velocityY) {
                if (responder == null || !responder.onFling(posX, posY, velocityX, velocityY)) {
                    nativeHandleFlingGesture(posX, posY, velocityX, velocityY);
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
                    nativeHandleRotateGesture(x, y, rotation);
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
                    nativeHandlePinchGesture(x, y, scale, velocity);
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
                    nativeHandleShoveGesture(distance);
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
            nativePickFeature(posX, posY, featurePickListener);
        }
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

    // Native methods
    // ==============

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tangram");
    }

    private synchronized native void nativeInit(MapController instance, AssetManager assetManager, String stylePath);
    private synchronized native void nativeLoadScene(String path);
    private synchronized native void nativeSetupGL();
    private synchronized native void nativeResize(int width, int height);
    private synchronized native void nativeUpdate(float dt);
    private synchronized native void nativeRender();
    private synchronized native void nativeSetPosition(double lon, double lat);
    private synchronized native void nativeSetPositionEased(double lon, double lat, float duration, int ease);
    private synchronized native void nativeGetPosition(double[] lonLatOut);
    private synchronized native void nativeSetZoom(float zoom);
    private synchronized native void nativeSetZoomEased(float zoom, float duration, int ease);
    private synchronized native float nativeGetZoom();
    private synchronized native void nativeSetRotation(float radians);
    private synchronized native void nativeSetRotationEased(float radians, float duration, int ease);
    private synchronized native float nativeGetRotation();
    private synchronized native void nativeSetTilt(float radians);
    private synchronized native void nativeSetTiltEased(float radians, float duration, int ease);
    private synchronized native float nativeGetTilt();
    private synchronized native void nativeScreenToWorldCoordinates(double[] screenCoords);
    private synchronized native void nativeSetPixelScale(float scale);
    private synchronized native void nativeSetCameraType(int cameraType);
    private synchronized native int nativeGetCameraType();
    private synchronized native void nativeHandleTapGesture(float posX, float posY);
    private synchronized native void nativeHandleDoubleTapGesture(float posX, float posY);
    private synchronized native void nativeHandlePanGesture(float startX, float startY, float endX, float endY);
    private synchronized native void nativeHandleFlingGesture(float posX, float posY, float velocityX, float velocityY);
    private synchronized native void nativeHandlePinchGesture(float posX, float posY, float scale, float velocity);
    private synchronized native void nativeHandleRotateGesture(float posX, float posY, float rotation);
    private synchronized native void nativeHandleShoveGesture(float distance);

    public synchronized native void nativePickFeature(float posX, float posY, FeaturePickListener listener);

    private native void nativeOnUrlSuccess(byte[] rawDataBytes, long callbackPtr);
    private native void nativeOnUrlFailure(long callbackPtr);

    native long nativeAddDataSource(String name);
    native void nativeRemoveDataSource(long pointer);
    native void nativeClearDataSource(long pointer);
    native void nativeAddFeature(long pointer, double[] coordinates, int[] rings, String[] properties);
    native void nativeAddGeoJson(long pointer, String geojson);

    native void nativeSetDebugFlag(int flag, boolean on);

    // Private members
    // ===============

    private String scenePath;
    private long time = System.nanoTime();
    private GLSurfaceView mapView;
    private AssetManager assetManager;
    private TouchInput touchInput;
    private FontFileParser fontFileParser;
    private DisplayMetrics displayMetrics = new DisplayMetrics();
    private HttpHandler httpHandler;
    private FeaturePickListener featurePickListener;

    // GLSurfaceView.Renderer methods
    // ==============================

    @Override
    public void onDrawFrame(GL10 gl) {
        long newTime = System.nanoTime();
        float delta = (newTime - time) / 1000000000.0f;
        time = newTime;

        nativeUpdate(delta);
        nativeRender();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        nativeSetPixelScale(displayMetrics.density);
        nativeResize(width, height);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeInit(this, assetManager, scenePath);
        // nativeInit() is safe to call twice, this invocation ensures that the jni
        // environment is attached to the rendering thread
        nativeSetupGL();
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
                //e.printStackTrace();
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

    public String getFontFallbackFilePath(int importance, int weightHint) {

        return fontFileParser.getFontFallback(importance, weightHint);
    }

}

