package com.mapzen.tangram;

import android.app.Activity;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.DisplayMetrics;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.GestureDetector.OnDoubleTapListener;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;
import android.view.View;
import android.view.View.OnTouchListener;

import com.mapzen.tangram.FontFileParser;

import com.almeros.android.multitouch.RotateGestureDetector;
import com.almeros.android.multitouch.RotateGestureDetector.OnRotateGestureListener;
import com.almeros.android.multitouch.ShoveGestureDetector;
import com.almeros.android.multitouch.ShoveGestureDetector.OnShoveGestureListener;
import com.squareup.okhttp.Cache;
import com.squareup.okhttp.Callback;
import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.Response;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import okio.BufferedSource;

import org.xmlpull.v1.XmlPullParserException;

public class MapController implements Renderer, OnTouchListener, OnScaleGestureListener, OnRotateGestureListener, OnGestureListener, OnDoubleTapListener, OnShoveGestureListener {

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

        // Set up gesture recognizers
        gestureDetector = new GestureDetector(mainApp, this);
        scaleGestureDetector = new ScaleGestureDetector(mainApp, this);
        rotateGestureDetector = new RotateGestureDetector(mainApp, this);
        shoveGestureDetector = new ShoveGestureDetector(mainApp, this);
        gestureDetector.setOnDoubleTapListener(this);

        // Set up okHTTP
        okRequestBuilder = new Request.Builder();
        okClient = new OkHttpClient();
        okClient.setConnectTimeout(10, TimeUnit.SECONDS);
        okClient.setReadTimeout(30, TimeUnit.SECONDS);

        // Set up MapView
        mapView = view;
        view.setOnTouchListener(this);
        view.setRenderer(this);
        view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

    }

    /**
     * Cache map data in a directory with a specified size limit
     * @param directory Directory in which map data will be cached
     * @param maxSize Maximum size of data to cache, in bytes
     * @return true if cache was successfully created
     */
    public boolean setTileCache(File directory, long maxSize) {
        try {
            Cache okTileCache = new Cache(directory, maxSize);
            okClient.setCache(okTileCache);
        } catch (IOException ignored) { return false; }
        return true;
    }

    /**
     * Set the geographic position of the center of the map view
     * @param lon Degrees longitude of the position to set
     * @param lat Degrees latitude of the position to set
     */
    public void setMapPosition(double lon, double lat) {
        mapLonLat[0] = lon;
        mapLonLat[1] = lat;
        if (initialized) { setPosition(lon, lat); }
    }

    /**
     * Get the geographic position of the center of the map view
     * @return Degrees longitude and latitude of the current map position, in a two-element array
     */
    public double[] getMapPosition() {
        return getMapPosition(new double[2]);
    }

    /**
     * Get the geographic position of the center of the map view
     * @param coordinatesOut Two-element array to be returned as the result
     * @return Degrees longitude and latitude of the current map position, in a two-element array
     */
    public double[] getMapPosition(double[] coordinatesOut) {
        if (initialized) { getPosition(mapLonLat); }
        coordinatesOut[0] = mapLonLat[0];
        coordinatesOut[1] = mapLonLat[1];
        return coordinatesOut;
    }

    /**
     * Set the zoom level of the map view
     * @param zoom Fractional zoom level
     */
    public void setMapZoom(float zoom) {
        mapZoom = zoom;
        if (initialized) { setZoom(zoom); }
    }

    /**
     * Get the zoom level of the map view
     * @return Fractional zoom level
     */
    public float getMapZoom() {
        if (initialized) { mapZoom = getZoom(); }
        return mapZoom;
    }

    /**
     * Set the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
     * @param radians Rotation in radians
     */
    public void setMapRotation(float radians) {
        mapRotation = radians;
        if (initialized) { setRotation(radians); }
    }

    /**
     * Get the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
     * @return Rotation in radians
     */
    public float getMapRotation() {
        if (initialized) { mapRotation = getRotation(); }
        return mapRotation;
    }

    /**
     * Set the tilt angle of the view in radians; 0 corresponds to straight down
     * @param radians Tilt angle in radians
     */
    public void setMapTilt(float radians) {
        mapTilt = radians;
        if (initialized) { setTilt(radians); }
    }

    /**
     * Get the tilt angle of the view in radians; 0 corresponds to straight down
     * @return Tilt angle in radians
     */
    public float getMapTilt() {
        if (initialized) { mapTilt = getTilt(); }
        return mapTilt;
    }

    /**
     * Find the geographic coordinates corresponding to the given position on screen
     * @param screenX Pixels from the left edge of the screen
     * @param screenY Pixels from the top edge of the screen
     * @return Degrees longitude and latitude corresponding to the given point, in a two-element array
     */
    public double[] coordinatesAtScreenPosition(double screenX, double screenY) {
        return coordinatesAtScreenPosition(new double[] {screenX, screenY});
    }

    /**
     * Find geographic coordinates corrseponding to the given position on screen
     * @param coordinatesInOut Two-element array of the x and y screen coordinates in pixels, the same
     * array will be returned with the values modified
     * @return Degrees longitude and latitude corresponding to the given point, in a two-element array
     */
    public double[] coordinatesAtScreenPosition(double[] coordinatesInOut) {
        if (initialized) { screenToWorldCoordinates(coordinatesInOut); }
        return coordinatesInOut;
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
     * Set a listener for long press events
     * @param listener Listener to call
     */
    public void setLongPressListener(View.OnGenericMotionListener listener) {
        longPressListener = listener;
    }

    /**
     * Set a listener for tap gesture
     * @param listener Listen to call
     */
    public void setTapGestureListener(View.OnGenericMotionListener listener) {
        tapGestureListener = listener;
    }

    // Native methods
    // ==============

    private synchronized native void init(MapController instance, AssetManager assetManager, String stylePath);
    private synchronized native void resize(int width, int height);
    private synchronized native void update(float dt);
    private synchronized native void render();
    private synchronized native void setPosition(double lon, double lat);
    private synchronized native void getPosition(double[] lonLatOut);
    private synchronized native void setZoom(float zoom);
    private synchronized native float getZoom();
    private synchronized native void setRotation(float radians);
    private synchronized native float getRotation();
    private synchronized native void setTilt(float radians);
    private synchronized native float getTilt();
    private synchronized native void screenToWorldCoordinates(double[] screenCoords);
    private synchronized native void onContextDestroyed();
    private synchronized native void setPixelScale(float scale);
    private synchronized native void handleTapGesture(float posX, float posY);
    private synchronized native void handleDoubleTapGesture(float posX, float posY);
    private synchronized native void handlePanGesture(float startX, float startY, float endX, float endY);
    private synchronized native void handlePinchGesture(float posX, float posY, float scale, float velocity);
    private synchronized native void handleRotateGesture(float posX, float posY, float rotation);
    private synchronized native void handleShoveGesture(float distance);
    private synchronized native void onUrlSuccess(byte[] rawDataBytes, long callbackPtr);
    private synchronized native void onUrlFailure(long callbackPtr);

    // Private members
    // ===============

    private String scenePath;
    private long time = System.nanoTime();
    private boolean initialized = false;
    private double[] mapLonLat = {0, 0};
    private float mapZoom = 0;
    private float mapRotation = 0;
    private float mapTilt = 0;
    private MapView mapView;
    private AssetManager assetManager;
    private GestureDetector gestureDetector;
    private ScaleGestureDetector scaleGestureDetector;
    private RotateGestureDetector rotateGestureDetector;
    private ShoveGestureDetector shoveGestureDetector;

    private static final FontFileParser fontFileParser = new FontFileParser();

    private static final float PINCH_THRESHOLD = 0.015f; //1.5% of minDim
    private static final float ROTATION_THRESHOLD = 0.30f;
    private static final float DOUBLETAP_MOVE_THRESHOLD = 5.0f;
    private static final long SCROLL_TIME_THRESHOLD = 100; // Ignore small residual scrolls/pans post a double finger gesture

    private float doubleTapDownY;

    /*
     * NOTE: Not relying on gestureDetector.isInProgress() since this is set after gestureBegin call and before
     * onGesture call, which makes it hard to get exclusivity with gestures.
     */
    private boolean mScaleHandled = false;
    private boolean mRotationHandled = false;
    private boolean mShoveHandled = false;
    private boolean mDoubleTapScale = false;

    private long mLastDoubleGestureTime = -SCROLL_TIME_THRESHOLD;

    private View.OnGenericMotionListener longPressListener;
    private View.OnGenericMotionListener tapGestureListener;
    private DisplayMetrics displayMetrics = new DisplayMetrics();

    private OkHttpClient okClient;
    private Request.Builder okRequestBuilder;

    // View.OnTouchListener methods
    // ============================

    public boolean onTouch(View v, MotionEvent event) {

        gestureDetector.onTouchEvent(event);
        scaleGestureDetector.onTouchEvent(event);
        shoveGestureDetector.onTouchEvent(event);
        rotateGestureDetector.onTouchEvent(event);

        return true;
    }

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
        onContextDestroyed();
        init(this, assetManager, scenePath);
        setPosition(mapLonLat[0], mapLonLat[1]);
        setZoom(mapZoom);
        setRotation(mapRotation);
        setTilt(mapTilt);
        initialized = true;
    }

    // GestureDetector.OnDoubleTapListener methods
    // ========================================

    public boolean onDoubleTap(MotionEvent event) {
        return true;
    }

    public boolean onDoubleTapEvent(MotionEvent event) {
        if(event.getAction() == 2) { // Move action during double tap
            // Set TapScaling only if sufficient Y movement has happened
            float movement = event.getY() - doubleTapDownY;
            if(Math.abs(movement) > DOUBLETAP_MOVE_THRESHOLD) {
                mDoubleTapScale = true;
            }
        }
        else if(event.getAction() == 1) { // DoubleTap Up; handleDoubleTap zoom-in if not moved (scale happened)
            if(!mDoubleTapScale) {
                handleDoubleTapGesture(mapView.getWidth() * 0.5f, mapView.getHeight() * 0.5f);
            }
            mDoubleTapScale = false;
        } else { // DoubleTap down event
            doubleTapDownY = event.getY();
        }
        return true;
    }

    public boolean onSingleTapConfirmed(MotionEvent event) {
        if (tapGestureListener != null) {
            return tapGestureListener.onGenericMotion(mapView, event);
        }
        return false;
    }

    // GestureDetector.OnGestureListener methods
    // ========================================

    public boolean onDown(MotionEvent event) {
        handlePanGesture(0.0f, 0.0f, 0.0f, 0.0f);
        return true;
    }

    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {

        // Ignore onScroll for a small time period after a double finger gesture has occured
        // Depending on how fingers are picked up after a double finger gesture, there could be a residual "single"
        // finger pan which could also result in a fling. This check will ignore that.
        long time = e2.getEventTime();
        if( ( (time - mLastDoubleGestureTime) < SCROLL_TIME_THRESHOLD) && e2.getPointerCount() == 1) {
            return false;
        }

        if (mShoveHandled) { return false; }

        float x = 0, y = 0;
        int n = e2.getPointerCount();
        for (int i = 0; i < n; i++) {
          x += e2.getX(i) / n;
          y += e2.getY(i) / n;
        }

        handlePanGesture(x + distanceX, y + distanceY, x, y);

        return true;
    }

    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        //not handled
        return false;
    }

    public void onLongPress(MotionEvent event) {
        if (longPressListener != null) {
            longPressListener.onGenericMotion(mapView, event);
        }
    }

    public void onShowPress(MotionEvent event) {
        //not handled
    }

    public boolean onSingleTapUp(MotionEvent event) {
        return true;
    }

    // ScaleGestureDetector.OnScaleGestureListener methods
    // ===================================================

    public boolean onScaleBegin(ScaleGestureDetector detector) {
        mScaleHandled = false;
        return !mShoveHandled;
    }

    public boolean onScale(ScaleGestureDetector detector) {
        // the velocity of the pinch in scale factor per ms
        float velocity = (detector.getCurrentSpan() - detector.getPreviousSpan()) / detector.getTimeDelta();
        float currentSpan = detector.getCurrentSpanX();
        float prevSpan = detector.getPreviousSpanX();

        double diff = Math.abs(currentSpan - prevSpan);

         /*
          * If Shove is in progress do not handle scale
          * If previous scale is handled then keep on handling scale
          * else give some buffer for shove to be processed
          */
        if ( mDoubleTapScale || mScaleHandled || (!mShoveHandled && diff > PINCH_THRESHOLD * Math.min(displayMetrics.widthPixels, displayMetrics.heightPixels) )) {
            mScaleHandled = true;
            float focusX = mDoubleTapScale ? mapView.getWidth() * 0.5f : detector.getFocusX();
            float focusY = mDoubleTapScale ? mapView.getHeight() * 0.5f : detector.getFocusY();
            mLastDoubleGestureTime = detector.getEventTime();
            handlePinchGesture(focusX, focusY, detector.getScaleFactor(), velocity);
            return true;
        }
        mScaleHandled = false;
        return false;
    }

    public void onScaleEnd(ScaleGestureDetector detector) {
        mScaleHandled = false;
    }

    // RotateGestureDetector.OnRotateGestureListener methods
    // =====================================================

    public boolean onRotateBegin(RotateGestureDetector detector) {
        mRotationHandled = false;
        return !mShoveHandled;
    }

    public boolean onRotate(RotateGestureDetector detector) {

        float rotation = detector.getRotationRadiansDelta();

         /*
          * If Shove is in progress do not handle rotation
          * If previous rotation is handled then keep on handling it
          * else give some buffer for shove to be processed
          */
        if ( mRotationHandled || (!mShoveHandled && (Math.abs(rotation) > ROTATION_THRESHOLD)) ) {
            float rotAngle;

            // Compensate for the ROTATION_THRESHOLD at the very beginning of the gesture, to avoid rotation jumps
            if (!mRotationHandled) {
                rotAngle = (rotation > 0) ? (rotation- ROTATION_THRESHOLD) : (rotation + ROTATION_THRESHOLD);
            } else {
                rotAngle = rotation;
            }
            float x = rotateGestureDetector.getFocusX();
            float y = rotateGestureDetector.getFocusY();
            mLastDoubleGestureTime = detector.getEventTime();
            handleRotateGesture(x, y, -(rotAngle));
            mRotationHandled = true;
            return true;
        }
        mRotationHandled = false;
        return false;
    }

    public void onRotateEnd(RotateGestureDetector detector) {
        mRotationHandled = false;
    }

    // ShoveGestureDetector.OnShoveGestureListener methods
    // ===================================================

    public boolean onShoveBegin(ShoveGestureDetector detector) {
        // If scale has started being handled ignore shove
        mShoveHandled = false;
        return !(mScaleHandled && mRotationHandled);
    }

    public boolean onShove(ShoveGestureDetector detector) {

        if (mScaleHandled || mRotationHandled) return false;

        mShoveHandled = true;

        float currentSpanX = detector.getCurrentSpanX();
        float prevSpanX = detector.getPreviousSpanX();

        double diffX = Math.abs(currentSpanX - prevSpanX);
        mLastDoubleGestureTime = detector.getEventTime();
        handleShoveGesture(detector.getShovePixelsDelta() / displayMetrics.heightPixels);
        return true;
    }

    public void onShoveEnd(ShoveGestureDetector detector) {
        mShoveHandled = false;
    }

    // Networking methods
    // ==================

    public void cancelUrlRequest(String url) {
        okClient.cancel(url);
    }

    public String getFontFilePath(String family, String weight, String style) {

        return fontFileParser.getFontFile(family + "_" + weight + "_" + style);

    }

    public boolean startUrlRequest(String url, final long callbackPtr) throws Exception {
        Request request = okRequestBuilder.tag(url).url(url).build();

        okClient.newCall(request).enqueue(new Callback() {
            @Override
            public void onFailure(Request request, IOException e) {

                onUrlFailure(callbackPtr);
                e.printStackTrace();
            }

            @Override
            public void onResponse(Response response) throws IOException {

                if (!response.isSuccessful()) {
                    onUrlFailure(callbackPtr);
                    throw new IOException("Unexpected code " + response);
                }
                BufferedSource src = response.body().source();
                byte[] rawDataBytes = src.readByteArray();
                onUrlSuccess(rawDataBytes, callbackPtr);
            }
        });
        return true;
    }
}

