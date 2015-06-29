package com.mapzen.tangram;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;
import android.view.SurfaceHolder;
import com.almeros.android.multitouch.RotateGestureDetector;
import com.almeros.android.multitouch.RotateGestureDetector.OnRotateGestureListener;
import com.almeros.android.multitouch.ShoveGestureDetector;
import com.almeros.android.multitouch.ShoveGestureDetector.OnShoveGestureListener;

import com.squareup.okhttp.Callback;
import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.Cache;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.RequestBody;
import com.squareup.okhttp.Response;
import com.squareup.okhttp.Call;
import okio.BufferedSource;

public class Tangram extends GLSurfaceView implements Renderer, OnTouchListener, OnScaleGestureListener, OnRotateGestureListener, OnGestureListener, OnShoveGestureListener {

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tangram");
    }

    private static native void init(Tangram instance, AssetManager assetManager);
    private static native void resize(int width, int height);
    private static native void update(float dt);
    private static native void render();
    private static native void teardown();
    private static native void onContextDestroyed();
    private static native void setPixelScale(float scale);
    private static native void handleTapGesture(float posX, float posY);
    private static native void handleDoubleTapGesture(float posX, float posY);
    private static native void handlePanGesture(float startX, float startY, float endX, float endY);
    private static native void handlePinchGesture(float posX, float posY, float scale);
    private static native void handleRotateGesture(float posX, float posY, float rotation);
    private static native void handleShoveGesture(float distance);
    private static native void onUrlSuccess(byte[] rawDataBytes, long callbackPtr);
    private static native void onUrlFailure(long callbackPtr);

    private long time = System.nanoTime();
    private boolean contextDestroyed = false;
    private AssetManager assetManager;
    private GestureDetector gestureDetector;
    private ScaleGestureDetector scaleGestureDetector;
    private RotateGestureDetector rotateGestureDetector;
    private ShoveGestureDetector shoveGestureDetector;
    private DisplayMetrics displayMetrics = new DisplayMetrics();

    private OkHttpClient okClient;
    private Request.Builder okRequestBuilder;
    private static final int TILE_CACHE_SIZE = 1024 * 1024 * 30; // 30 MB

    public Tangram(Context context) {

        super(context);

        configureGLSurfaceView();        
   
    }

    public Tangram(Context context, AttributeSet attrs) {

        super(context, attrs);

        configureGLSurfaceView();

    }

    private void configureGLSurfaceView() {

        setOnTouchListener(this);
        setEGLContextClientVersion(2);
        setPreserveEGLContextOnPause(true);
        setEGLConfigChooser(8, 8, 8, 8, 24, 0);
        setRenderer(this);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

    }

    public void setup(Activity mainApp) {

        mainApp.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);

        assetManager = mainApp.getAssets();
        gestureDetector = new GestureDetector(mainApp, this);
        scaleGestureDetector = new ScaleGestureDetector(mainApp, this);
        rotateGestureDetector = new RotateGestureDetector(mainApp, this);
        shoveGestureDetector = new ShoveGestureDetector(mainApp, this);

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

    }

    public void onDestroy() {
        onContextDestroyed();
        teardown();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        contextDestroyed = true;
        super.surfaceDestroyed(holder);
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

        if (contextDestroyed) {
            onContextDestroyed();
            contextDestroyed = false;
        }

        init(this, assetManager);
    }

    // GestureDetetor.OnGestureListener methods
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
        return;
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
        return;
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
        return;
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

