package com.mapzen.tangram;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;
import android.view.SurfaceHolder;

public class Tangram extends GLSurfaceView implements Renderer, OnScaleGestureListener, OnGestureListener {

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tangram");
    }

    private static native void init(AssetManager assetManager);
    private static native void resize(int width, int height);
    private static native void update(float dt);
    private static native void render();
    private static native void teardown();
    private static native void onContextDestroyed();
    private static native void setPixelScale(float scale);
    private static native void handleTapGesture(float posX, float posY);
    private static native void handleDoubleTapGesture(float posX, float posY);
    private static native void handlePanGesture(float velX, float velY);
    private static native void handlePinchGesture(float posX, float posY, float scale);

    private long time = System.nanoTime();
    private float scaleFactor = 1.0f;
    private float scalePosX = 0.0f;
    private float scalePosY = 0.0f;
    private boolean contextDestroyed = false;
    private AssetManager assetManager;
    private GestureDetector gestureDetector;
    private ScaleGestureDetector scaleGestureDetector;
    private DisplayMetrics displayMetrics = new DisplayMetrics();

    public Tangram(Activity mainApp) {
        super(mainApp);
        
        setEGLContextClientVersion(2);
        setRenderer(this);
        setPreserveEGLContextOnPause(true);
        
        mainApp.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        
        this.assetManager = mainApp.getAssets();
        this.gestureDetector = new GestureDetector(mainApp, this);
        this.scaleGestureDetector = new ScaleGestureDetector(mainApp, this);
        
    }
    
    @Override
    public void onResume() {
        super.onResume();
    }
    
    public void onDestroy() {
        teardown();
    }
    
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        contextDestroyed = true;
        super.surfaceDestroyed(holder);
    }
    
    @Override
    public boolean onTouchEvent(MotionEvent event) { 
        
        //Pass the event to gestureDetector and scaleDetector
        boolean retVal;
        retVal = scaleGestureDetector.onTouchEvent(event);
        if (!scaleGestureDetector.isInProgress()) {
            retVal = gestureDetector.onTouchEvent(event);
            if (!gestureDetector.onTouchEvent(event)) {
                retVal = super.onTouchEvent(event);
            }
        }
        return retVal;
        
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
        
        init(assetManager);
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
        
        // We flip the signs of distanceX and distanceY because onScroll provides the distances
        // by which the view being scrolled should move, while handlePanGesture expects the 
        // distances by which the touch point has moved on the screen (these are opposite)
        handlePanGesture(-distanceX, -distanceY);
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
        scaleFactor = 1.0f;
        scalePosX = detector.getFocusX();
        scalePosY = detector.getFocusY();
        return true;
    }

    public boolean onScale(ScaleGestureDetector detector) {
        scaleFactor = detector.getScaleFactor() * scaleFactor;
        handlePinchGesture(scalePosX, scalePosY, scaleFactor);
        Log.v("\nPinch: ", scaleFactor + ",\t" + scalePosX + "," + scalePosY);
        return true;
    }

    public void onScaleEnd(ScaleGestureDetector detector) {
        // Only process the pinch gesture at the end (discrete zoom)
        //handlePinchGesture(scalePosX, scalePosY, scaleFactor);
        scaleFactor = 1.0f;
    }
}

