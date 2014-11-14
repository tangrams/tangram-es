package com.mapzen.tangram;

import android.util.Log;
import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLES10;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;
import android.support.v4.view.GestureDetectorCompat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;

public class Tangram implements GLSurfaceView.Renderer, ScaleGestureDetector.OnScaleGestureListener, GestureDetector.OnGestureListener {
	
    private float[] viewCenter;
    private float scaleFactor;
    private float scalePosX;
    private float scalePosY;

	static {
		System.loadLibrary("c++_shared");
		System.loadLibrary("tangram");
	}

	private static native void init(AssetManager assetManager);
	private static native void resize(int width, int height);
	private static native void render();
    private static native void update(float dt);
    private static native void handleGestures(int gestureType, float posOrVelxd, float posOrVely, float scale);

	private long time = System.nanoTime();

    private AssetManager assetManager;

    public Tangram(Context mainApp) {
        this.assetManager = mainApp.getAssets();
    }

    public void onDrawFrame(GL10 gl) 
    {
        long newTime = System.nanoTime();
        float delta = (newTime - time) / 1000000000.0f;
        time = newTime;

        update(delta);
        render();
    }

	public void onSurfaceChanged(GL10 gl, int width, int height) {
        //set the view center for gesture handling
        viewCenter = new float[2];
        viewCenter[0] = (float)width * 0.5f;
        viewCenter[1] = (float)height * 0.5f;
		resize(width, height);
	}

	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		init(assetManager);
	}

    public void constructGestures(int gestureType, float posOrVelx, float posOrVely) {
        handleGestures(gestureType, posOrVelx, posOrVely, 1.0f);
    }

    public void constructGestures(int gestureType, float posOrVelx, float posOrVely, float scale) {
        handleGestures(gestureType, posOrVelx, posOrVely, scale);
    }

    // Interface methods for OnGestureListener
    public boolean onDown(MotionEvent event) {
        return true;
    }
    
    public boolean onSingleTapConfirmed(MotionEvent event) {
        float touchX = event.getX();
        float touchY = event.getY();
        Log.v("onSingleTap", touchX + "," +touchY+"\t"+viewCenter[0]+","+viewCenter[1]);
        constructGestures(0, touchX - viewCenter[0], -(touchY - viewCenter[1]));
        return true;
    }

    public boolean onDoubleTap(MotionEvent event) {
        float touchX = event.getX();
        float touchY = event.getY();
        Log.v("onSingleTap", touchX + "," +touchY+"\t"+viewCenter[0]+","+viewCenter[1]);
        constructGestures(1, touchX - viewCenter[0], -(touchY - viewCenter[1]));
        return true;
    }

    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        //TODO: Better velocity calculations using VelocityTracker and VelocityTrackerCompat classes
        float touchX, touchY, prevTouchX, prevTouchY;
        float velocityX, velocityY;
        long time, prevTime;
        float invTimeDiff;
        
        touchX = e2.getX();
        touchY = e2.getY();
        time = e2.getEventTime();
        
        if(e2.getHistorySize() > 0) {
            prevTouchX = e2.getHistoricalX(0);
            prevTouchY = e2.getHistoricalY(0);
            prevTime = e2.getHistoricalEventTime(0);
        } else {
            prevTouchX = e1.getX();
            prevTouchY = e1.getY();
            prevTime = e1.getEventTime();
        }
        if( (time - prevTime) == 0) {
            return false;
        }
        
        // Factor of 100 to match iOS velocity behavior
        invTimeDiff = 100.0f/(float)(time - prevTime);
        velocityX = (touchX - prevTouchX) * (invTimeDiff);
        velocityY = (touchY - prevTouchY) * (invTimeDiff);
        
        Log.v("onPanTap", touchX + "," +touchY+"\t"+prevTouchX+","+prevTouchY);
        Log.v("\nonPanTap time:", time + "," + prevTime);
        Log.v("\nonPanTap Velocity:", velocityX + "," + velocityY);
        constructGestures(2, velocityX, velocityY);
        return true;
    }

    // Need this implemented as Tangram implements the OnGestureListener interface
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        //not handled so return false
        return false;
    }

    // Need this implemented as Tangram implements the OnGestureListener interface
    public void onLongPress(MotionEvent event) {
        //not handled so return false
    }

    // Need this implemented as Tangram implements the OnGestureListener interface
    public void onShowPress(MotionEvent event) {
        //not handled so return false
    }

    public boolean onSingleTapUp(MotionEvent event) {
        float touchX = event.getX();
        float touchY = event.getY();
        Log.v("onSingleTap", touchX + "," +touchY+"\t"+viewCenter[0]+","+viewCenter[1]);
        constructGestures(0, touchX - viewCenter[0], -(touchY - viewCenter[1]));
        return true;
    }

    // Interface methods for OnScaleGestureListener
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        scaleFactor = 1.0f;
        scalePosX = detector.getFocusX();
        scalePosY = detector.getFocusY();
        return true;
    }

    public boolean onScale(ScaleGestureDetector detector) {
        scaleFactor = detector.getScaleFactor() * scaleFactor;
        Log.v("\nPinch: ", scaleFactor + ",\t" + scalePosX + "," + scalePosY);
        // TODO: continous zoom
        //constructGestures(3, scalePosX, scalePosY, scaleFactor);
        return true;
    }

    public void onScaleEnd(ScaleGestureDetector detector) {
        // Only process the pinch gesture at the end (discrete zoom)
        constructGestures(3, scalePosX, scalePosY, scaleFactor);
        scaleFactor = 1.0f;
    }
}

