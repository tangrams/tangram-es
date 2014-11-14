package com.mapzen.tangram;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.util.Log;

import android.support.v4.view.GestureDetectorCompat;
import android.view.ScaleGestureDetector;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;

public class MainActivity extends Activity
{

    private GLSurfaceView view;
    private GestureDetectorCompat detector;
    private ScaleGestureDetector scaleDetector;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

		view = new GLSurfaceView(getApplication());
		view.setEGLContextClientVersion(2);
        view.setEGLConfigChooser(8,8,8,8,16,0);
        Tangram tangram = new Tangram();
        // tangram implements the interface: GLSurfaceView.Renderer e
		view.setRenderer(tangram);
		setContentView(view);
        //create an instance of gesture detectors
        //      tangram implements OnScaleGestureListener and OnGestureListener
        detector = new GestureDetectorCompat(this, tangram);
        scaleDetector = new ScaleGestureDetector(this, tangram);
    }

    @Override 
    public boolean onTouchEvent(MotionEvent event) 
    { 
        //Pass the event to gestureDetector and scaleDetector
        boolean retVal = this.scaleDetector.onTouchEvent(event);
        retVal = retVal || this.detector.onTouchEvent(event);
        return retVal || super.onTouchEvent(event);
    }

    @Override
	protected void onPause() 
	{
		super.onPause();
		view.onPause();
	}

	@Override
	protected void onResume() 
	{
		super.onResume();
		view.onResume();
	}

}

