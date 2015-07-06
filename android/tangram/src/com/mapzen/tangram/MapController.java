package com.mapzen.tangram;

import android.app.Activity;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.DisplayMetrics;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;
import android.view.View;
import android.view.View.OnTouchListener;

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

public class MapController implements Renderer, OnTouchListener, OnScaleGestureListener, OnRotateGestureListener, OnGestureListener, OnShoveGestureListener {

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tangram");
    }

    public void setMapPosition(double lon, double lat) {
        mapLonLat[0] = lon;
        mapLonLat[1] = lat;
        if (initialized) { setPosition(lon, lat); }
    }

    public double[] getMapPosition() {
        return getMapPosition(new double[2]);
    }

    public double[] getMapPosition(double[] coordinatesOut) {
        if (initialized) { getPosition(mapLonLat); }
        coordinatesOut[0] = mapLonLat[0];
        coordinatesOut[1] = mapLonLat[1];
        return coordinatesOut;
    }

    public void setMapZoom(float zoom) {
        mapZoom = zoom;
        if (initialized) { setZoom(zoom); }
    }

    public float  getMapZoom() {
        if (initialized) { mapZoom = getZoom(); }
        return mapZoom;
    }

    public void setMapRotation(float radians) {
        mapRotation = radians;
        if (initialized) { setRotation(radians); }
    }

    public float getMapRotation() {
        if (initialized) { mapRotation = getRotation(); }
        return mapRotation;
    }

    public void setMapTilt(float radians) {
        mapTilt = radians;
        if (initialized) { setTilt(radians); }
    }

    public float getMapTilt() {
        if (initialized) { mapTilt = getTilt(); }
        return mapTilt;
    }

    private native void init(MapController instance, AssetManager assetManager);
    private native void resize(int width, int height);
    private native void update(float dt);
    private native void render();
    private native void setPosition(double lon, double lat);
    private native void getPosition(double[] lonLatOut);
    private native void setZoom(float zoom);
    private native float getZoom();
    private native void setRotation(float radians);
    private native float getRotation();
    private native void setTilt(float radians);
    private native float getTilt();
    private native void onContextDestroyed();
    private native void setPixelScale(float scale);
    private native void handleTapGesture(float posX, float posY);
    private native void handleDoubleTapGesture(float posX, float posY);
    private native void handlePanGesture(float startX, float startY, float endX, float endY);
    private native void handlePinchGesture(float posX, float posY, float scale);
    private native void handleRotateGesture(float posX, float posY, float rotation);
    private native void handleShoveGesture(float distance);
    private native void onUrlSuccess(byte[] rawDataBytes, long callbackPtr);
    private native void onUrlFailure(long callbackPtr);

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
    private DisplayMetrics displayMetrics = new DisplayMetrics();

    private OkHttpClient okClient;
    private Request.Builder okRequestBuilder;
    private static final int TILE_CACHE_SIZE = 1024 * 1024 * 30; // 30 MB

    public MapController(Activity mainApp, MapView view) {

        // Get configuration info from application
        mainApp.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        assetManager = mainApp.getAssets();

        // Set up gesture recognizers
        gestureDetector = new GestureDetector(mainApp, this);
        scaleGestureDetector = new ScaleGestureDetector(mainApp, this);
        rotateGestureDetector = new RotateGestureDetector(mainApp, this);
        shoveGestureDetector = new ShoveGestureDetector(mainApp, this);

        // Set up okHTTP
        okRequestBuilder = new Request.Builder();
        okClient = new OkHttpClient();
        okClient.setConnectTimeout(10, TimeUnit.SECONDS);
        okClient.setReadTimeout(30, TimeUnit.SECONDS);
        try {
            File cacheDir = new File(mainApp.getExternalCacheDir().getAbsolutePath() + "/tile_cache");
            Cache okTileCache = new Cache(cacheDir, TILE_CACHE_SIZE);
            okClient.setCache(okTileCache);
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Set up MapView
        mapView = view;
        view.setOnTouchListener(this);
        view.setRenderer(this);
        view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

    }

    public void requestRender() {
        mapView.requestRender();
    }

    public void setRenderMode(int renderMode) {
        mapView.setRenderMode(renderMode);
    }

    // View.OnTouchListener methods
    // ============================

    public boolean onTouch(View v, MotionEvent event) {

        gestureDetector.onTouchEvent(event);
        shoveGestureDetector.onTouchEvent(event);
        scaleGestureDetector.onTouchEvent(event);
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
        init(this, assetManager);
        setPosition(mapLonLat[0], mapLonLat[1]);
        setZoom(mapZoom);
        setRotation(mapRotation);
        setTilt(mapTilt);
        initialized = true;
    }

    // GestureDetector.OnGestureListener methods
    // ========================================

    public boolean onDown(MotionEvent event) {
        return true;
    }

    public boolean onDoubleTap(MotionEvent event) {
        handleDoubleTapGesture(event.getX(), event.getY());
        return true;
    }

    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        // Only pan for scrolling events with just one pointer; otherwise vertical scrolling will
        // cause a simultaneous shove gesture
        if (e1.getPointerCount() == 1 && e2.getPointerCount() == 1) {
            // We flip the signs of distanceX and distanceY because onScroll provides the distances
            // by which the view being scrolled should move, while handlePanGesture expects the
            // distances by which the touch point has moved on the screen (these are opposite)
            float x = e2.getX();
            float y = e2.getY();
            handlePanGesture(x + distanceX, y + distanceY, x, y);
        }
        return true;
    }

    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        //not handled
        return false;
    }

    public void onLongPress(MotionEvent event) {
        //not handled
    }

    public void onShowPress(MotionEvent event) {
        //not handled
    }

    public boolean onSingleTapUp(MotionEvent event) {
        handleTapGesture(event.getX(), event.getY());
        return true;
    }

    // ScaleGestureDetector.OnScaleGestureListener methods
    // ===================================================

    public boolean onScaleBegin(ScaleGestureDetector detector) {
        return !shoveGestureDetector.isInProgress();
    }

    public boolean onScale(ScaleGestureDetector detector) {
        handlePinchGesture(detector.getFocusX(), detector.getFocusY(), detector.getScaleFactor());
        return true;
    }

    public void onScaleEnd(ScaleGestureDetector detector) {
    }

    // RotateGestureDetector.OnRotateGestureListener methods
    // =====================================================

    public boolean onRotateBegin(RotateGestureDetector detector) {
        return !shoveGestureDetector.isInProgress();
    }

    public boolean onRotate(RotateGestureDetector detector) {
        float x = scaleGestureDetector.getFocusX();
        float y = scaleGestureDetector.getFocusY();
        float rotation = -detector.getRotationDegreesDelta() * (float)(Math.PI / 180);
        handleRotateGesture(x, y, rotation);
        return true;
    }

    public void onRotateEnd(RotateGestureDetector detector) {
    }

    // ShoveGestureDetector.OnShoveGestureListener methods
    // ===================================================

    public boolean onShoveBegin(ShoveGestureDetector detector) {
        return !(scaleGestureDetector.isInProgress() || rotateGestureDetector.isInProgress());
    }

    public boolean onShove(ShoveGestureDetector detector) {
        handleShoveGesture(detector.getShovePixelsDelta() / displayMetrics.heightPixels);
        return true;
    }

    public void onShoveEnd(ShoveGestureDetector detector) {
    }

    public void cancelUrlRequest(String url) {
        okClient.cancel(url);
    }

    // Network requests using okHttp
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

                if(!response.isSuccessful()) {
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

