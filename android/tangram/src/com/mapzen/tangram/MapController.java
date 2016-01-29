package com.mapzen.tangram;

import android.app.Activity;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.DisplayMetrics;

import com.squareup.okhttp.Callback;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.Response;

import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import okio.BufferedSource;

import com.mapzen.tangram.TouchInput.Gestures;

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

    public interface FeatureTouchListener {
        void onTouch(Properties properties, float positionX, float positionY);
    }

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tangram");
    }

    /**
     * Construct a MapController using the default scene file
     * @param mainApp Activity in which the map will function; the asset bundle for this activity must contain all
     * the local files that the map will need
     * @param view MapView where the map will be displayed; input events from this view will be handled by the
     * resulting MapController
     */
    public MapController(Activity mainApp, MapView view) {

        this(mainApp, view, "scene.yaml");
    }

    /**
     * Construct a MapController using a custom scene file
     * @param mainApp Activity in which the map will function; the asset bundle for this activity must contain all
     * the local files that the map will need
     * @param view MapView where the map will be displayed; input events from this view will be handled by the
     * resulting MapController
     * @param sceneFilePath Location of the YAML scene file within the assets directory
     */
    public MapController(Activity mainApp, MapView view, String sceneFilePath) {

        scenePath = sceneFilePath;

        // Get configuration info from application
        mainApp.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        assetManager = mainApp.getAssets();

        touchInput = new TouchInput(mainApp);
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

        // Load the fonts
        fontFileParser = new FontFileParser();
        fontFileParser.parse("/system/etc/fonts.xml");

        init(this, assetManager, scenePath);

    }

    public void loadSceneFile(String path) {
        scenePath = path;
        loadScene(path);
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
     * @param duration Time in seconds to ease to the given position
     * @param ease Type of easing to use
     */
    public void setMapPosition(double lng, double lat) {
        setPosition(lng, lat);
    }
    public void setMapPosition(double lng, double lat, float duration) {
        setPositionEased(lng, lat, duration, EaseType.QUINT.ordinal());
    }
    public void setMapPosition(double lng, double lat, float duration, EaseType ease) {
        setPositionEased(lng, lat, duration, ease.ordinal());
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
        getPosition(tmp);
        return out.set(tmp[0], tmp[1]);
    }

    /**
     * Set the zoom level of the map view
     * @param zoom Fractional zoom level
     * @param duration Time in seconds to ease to given zoom
     * @param ease Type of easing to use
     */
    public void setMapZoom(float zoom) {
        setZoom(zoom);
    }
    public void setMapZoom(float zoom, float duration) {
        setZoomEased(zoom, duration, EaseType.QUINT.ordinal());
    }
    public void setMapZoom(float zoom, float duration, EaseType ease) {
        setZoomEased(zoom, duration, ease.ordinal());
    }

    /**
     * Get the zoom level of the map view
     * @return Fractional zoom level
     */
    public float getMapZoom() {
        return getZoom();
    }

    /**
     * Set the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
     * @param radians Rotation in radians
     * @param duration Time in seconds to ease to the given rotation
     * @param ease Type of easing to use
     */
    public void setMapRotation(float radians) {
        setRotation(radians);
    }
    public void setMapRotation(float radians, float duration) {
        setRotationEased(radians, duration, EaseType.QUINT.ordinal());
    }
    public void setMapRotation(float radians, float duration, EaseType ease) {
        setRotationEased(radians, duration, ease.ordinal());
    }

    /**
     * Get the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
     * @return Rotation in radians
     */
    public float getMapRotation() {
        return getRotation();
    }

    /**
     * Set the tilt angle of the view in radians; 0 corresponds to straight down
     * @param radians Tilt angle in radians
     * @param duration Time in seconds to ease to the given tilt
     * @param ease Type of easing to use
     */
    public void setMapTilt(float radians) {
        setTilt(radians);
    }
    public void setMapTilt(float radians, float duration) {
        setTiltEased(radians, duration, EaseType.QUINT.ordinal());
    }
    public void setMapTilt(float radians, float duration, EaseType ease) {
        setTiltEased(radians, duration, ease.ordinal());
    }

    /**
     * Get the tilt angle of the view in radians; 0 corresponds to straight down
     * @return Tilt angle in radians
     */
    public float getMapTilt() {
        return getTilt();
    }

    public void setMapCameraType(CameraType _cameraType) {
        setCameraType(_cameraType.ordinal());
        requestRender();
    }

    public CameraType getMapCameraType() {
        return CameraType.values()[getCameraType()];
    }

    /**
     * Find the geographic coordinates corresponding to the given position on screen
     * @param screenX Pixels from the left edge of the screen
     * @param screenY Pixels from the top edge of the screen
     * @return LngLat corresponding to the given point
     */
    public LngLat coordinatesAtScreenPosition(double screenX, double screenY) {
        double[] tmp = { screenX, screenY };
        screenToWorldCoordinates(tmp);
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
                    handlePanGesture(startX, startY, endX, endY);
                }
                return true;
            }

            @Override
            public boolean onFling(float posX, float posY, float velocityX, float velocityY) {
                if (responder == null || !responder.onFling(posX, posY, velocityX, velocityY)) {
                    handleFlingGesture(posX, posY, velocityX, velocityY);
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
                    handleRotateGesture(x, y, rotation);
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
                    handlePinchGesture(x, y, scale, velocity);
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
                    handleShoveGesture(distance);
                }
                return true;
            }
        });
    }

    /**
     * Set whether the gesture {@param second} can be recognized while {@param first} is in progress
     * @param first Initial gesture type
     * @param second Subsequent gesture type
     * @param allowed True if {@param second} should be recognized, else false
     */
    public void setSimultaneousGestureAllowed(Gestures first, Gestures second, boolean allowed) {
        touchInput.setSimultaneousDetectionAllowed(first, second, allowed);
    }

    /**
     * Get whether the gesture {@param second} can be recognized while {@param first} is in progress
     * @param first Initial gesture type
     * @param second Subsequent gesture type
     * @return True if {@param second} will be recognized, else false
     */
    public boolean isSimultaneousGestureAllowed(Gestures first, Gestures second) {
        return touchInput.isSimultaneousDetectionAllowed(first, second);
    }

    /**
     * Set a listener for Feature touch events
     * @param listener Listen to call
     */
    public void setFeatureTouchListener(FeatureTouchListener listener) {
        featureTouchListener = listener;
    }

    /**
     * Enqueue a Runnable to be executed synchronously on the rendering thread
     * @param r Runnable to run
     */
    public void queueEvent(Runnable r) {
        mapView.queueEvent(r);
    }

    // Native methods
    // ==============

    private synchronized native void init(MapController instance, AssetManager assetManager, String stylePath);
    private synchronized native void loadScene(String path);
    private synchronized native void setupGL();
    private synchronized native void resize(int width, int height);
    private synchronized native void update(float dt);
    private synchronized native void render();
    private synchronized native void setPosition(double lon, double lat);
    private synchronized native void setPositionEased(double lon, double lat, float duration, int ease);
    private synchronized native void getPosition(double[] lonLatOut);
    private synchronized native void setZoom(float zoom);
    private synchronized native void setZoomEased(float zoom, float duration, int ease);
    private synchronized native float getZoom();
    private synchronized native void setRotation(float radians);
    private synchronized native void setRotationEased(float radians, float duration, int ease);
    private synchronized native float getRotation();
    private synchronized native void setTilt(float radians);
    private synchronized native void setTiltEased(float radians, float duration, int ease);
    private synchronized native float getTilt();
    private synchronized native void screenToWorldCoordinates(double[] screenCoords);
    private synchronized native void setPixelScale(float scale);
    private synchronized native void setCameraType(int cameraType);
    private synchronized native int getCameraType();
    private synchronized native void handleTapGesture(float posX, float posY);
    private synchronized native void handleDoubleTapGesture(float posX, float posY);
    private synchronized native void handlePanGesture(float startX, float startY, float endX, float endY);
    private synchronized native void handleFlingGesture(float posX, float posY, float velocityX, float velocityY);
    private synchronized native void handlePinchGesture(float posX, float posY, float scale, float velocity);
    private synchronized native void handleRotateGesture(float posX, float posY, float rotation);
    private synchronized native void handleShoveGesture(float distance);
    public synchronized native void pickFeature(float posX, float posY);

    private native void onUrlSuccess(byte[] rawDataBytes, long callbackPtr);
    private native void onUrlFailure(long callbackPtr);

    // Private members
    // ===============

    private static String TAG = "Tangram";
    private String scenePath;
    private long time = System.nanoTime();
    private MapView mapView;
    private AssetManager assetManager;
    private TouchInput touchInput;

    private FontFileParser fontFileParser;

    private DisplayMetrics displayMetrics = new DisplayMetrics();

    private HttpHandler httpHandler;

    private FeatureTouchListener featureTouchListener;

    // GLSurfaceView.Renderer methods
    // ==============================

    public void onDrawFrame(GL10 gl) {
        long newTime = System.nanoTime();
        float delta = (newTime - time) / 1000000000.0f;
        time = newTime;

        update(delta);
        render();
    }

    public void onSurfaceChanged(GL10 gl, int width, int height) {
        setPixelScale(displayMetrics.density);
        resize(width, height);
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        init(this, assetManager, scenePath);
        // init() is safe to call twice, this invocation ensures that the jni
        // environment is attached to the rendering thread
        setupGL();
    }

    // Networking methods
    // ==================

    public void cancelUrlRequest(String url) {
        if (httpHandler == null) {
            return;
        }
        httpHandler.onCancel(url);
    }

    public boolean startUrlRequest(String url, final long callbackPtr) throws Exception {
        if (httpHandler == null) {
            return false;
        }
        httpHandler.onRequest(url, new Callback() {
            @Override
            public void onFailure(Request request, IOException e) {
                onUrlFailure(callbackPtr);
                //e.printStackTrace();
            }

            @Override
            public void onResponse(Response response) throws IOException {
                if (!response.isSuccessful()) {
                    onUrlFailure(callbackPtr);
                    throw new IOException("Unexpected response code: " + response);
                }
                BufferedSource source = response.body().source();
                byte[] bytes = source.readByteArray();
                onUrlSuccess(bytes, callbackPtr);
            }
        });
        return true;
    }

    // Font Fetching
    // =============
    public String getFontFilePath(String key) {

        return fontFileParser.getFontFile(key);

    }

    public String getFontFallbackFilePath(int importance, int weightHint) {

        return fontFileParser.getFontFallback(importance, weightHint);
    }

    // Feature selection
    // =================
    public void featureSelectionCb(Properties properties, float positionX, float positionY) {
        if (featureTouchListener != null) {
            featureTouchListener.onTouch(properties, positionX, positionY);
        }
    }

}

