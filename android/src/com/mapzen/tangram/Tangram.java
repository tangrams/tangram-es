package com.mapzen.tangram;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
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

public class Tangram implements Renderer, OnTouchListener, OnScaleGestureListener, OnRotateGestureListener, OnGestureListener, OnShoveGestureListener {

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tangram");
    }

    private OkHttpClient okClient;
    private static final int TILE_CACHE_SIZE = 1024 * 1024 * 30; // 30 Mgs

    private static native void init(Tangram tangramInstance, AssetManager assetManager);
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
    private static native void networkDataBridge(byte[] rawDataBytes, int tileIDx, int tileIDy, int tileIDz, int dataSourceID);

    private long time = System.nanoTime();
    private boolean contextDestroyed = false;
    private AssetManager assetManager;
    private GestureDetector gestureDetector;
    private ScaleGestureDetector scaleGestureDetector;
    private RotateGestureDetector rotateGestureDetector;
    private ShoveGestureDetector shoveGestureDetector;
    private DisplayMetrics displayMetrics = new DisplayMetrics();
    private GLSurfaceView view;

    public Tangram(Activity mainApp) {
        
        view = new GLSurfaceView(mainApp) {

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                contextDestroyed = true;
                super.surfaceDestroyed(holder);
            }

        };
        
        view.setOnTouchListener(this);
        view.setEGLContextClientVersion(2);
        view.setPreserveEGLContextOnPause(true);
        view.setEGLConfigChooser(8, 8, 8, 8, 24, 0);
        view.setRenderer(this);
        view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        
        mainApp.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        
        this.assetManager = mainApp.getAssets();
        this.gestureDetector = new GestureDetector(mainApp, this);
        this.scaleGestureDetector = new ScaleGestureDetector(mainApp, this);
        this.rotateGestureDetector = new RotateGestureDetector(mainApp, this);
        this.shoveGestureDetector = new ShoveGestureDetector(mainApp, this);

        this.okClient = new OkHttpClient();
        okClient.setConnectTimeout(10, TimeUnit.SECONDS);
        okClient.setReadTimeout(30, TimeUnit.SECONDS);
        File cacheDir = new File(mainApp.getExternalCacheDir().getAbsolutePath() + "/tile_cache");
        try {
            Cache okTileCache = new Cache(cacheDir, TILE_CACHE_SIZE);
            okClient.setCache(okTileCache);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public View getView() {
        return view;
    }
    
    public void onDestroy() {
        teardown();
    }

    public void requestRender() {
        view.requestRender();
    }

    public void setRenderMode(int renderMode) {
        view.setRenderMode(renderMode);
    }
    
    // View.OnTouchListener methods
    // ============================

    public boolean onTouch(View v, MotionEvent event) { 
        
        //Pass the event to gesture detectors
        if (gestureDetector.onTouchEvent(event) |
            scaleGestureDetector.onTouchEvent(event) |
            rotateGestureDetector.onTouchEvent(event) |
            shoveGestureDetector.onTouchEvent(event)) {
            requestRender();
            return true;
        }
        
        return false;
        
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
        return true;
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
        return true;
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
        return true;
    }

    public boolean onShove(ShoveGestureDetector detector) {
        handleShoveGesture(detector.getShovePixelsDelta() / displayMetrics.heightPixels);
        return true;
    }

    public void onShoveEnd(ShoveGestureDetector detector) {
        return;
    }

    public void cancelNetworkRequest(String url) {
        okClient.cancel(url);
    }

    // Network requests using okHttp
    public boolean networkRequest(String url, final int tileIDx, final int tileIDy, final int tileIDz, final int dataSourceID) throws Exception {
        Request request = new Request.Builder().tag(url).url(url).build();

        okClient.newCall(request).enqueue(new Callback() {
            @Override 
            public void onFailure(Request request, IOException e) {
                e.printStackTrace();
            }

            @Override 
            public void onResponse(Response response) throws IOException {

                if(!response.isSuccessful()) throw new IOException("Unexpected code " + response);

                BufferedSource src = response.body().source();
                byte[] rawDataBytes = src.readByteArray();
                networkDataBridge(rawDataBytes, tileIDx, tileIDy, tileIDz, dataSourceID);
            }
        });
        return true;
    }
}

