package com.mapzen.tangram;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.Window;

public class MainActivity extends Activity
{

    private GLSurfaceView view;
    private GestureDetector detector;
    private ScaleGestureDetector scaleDetector;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

		view = new GLSurfaceView(getApplication());
		view.setEGLContextClientVersion(2);
        view.setEGLConfigChooser(8,8,8,8,16,0);
        Tangram tangram = new Tangram(this);
        // tangram implements the interface: GLSurfaceView.Renderer e
		view.setRenderer(tangram);
		setContentView(view);
        //create an instance of gesture detectors
        //      tangram implements OnScaleGestureListener and OnGestureListener
        detector = new GestureDetector(this, tangram);
        scaleDetector = new ScaleGestureDetector(this, tangram);
    }

    @Override 
    public boolean onTouchEvent(MotionEvent event) 
    { 
        //Pass the event to gestureDetector and scaleDetector
        boolean retVal;
        retVal = this.scaleDetector.onTouchEvent(event);
        if(!this.scaleDetector.isInProgress()) {
            retVal = this.detector.onTouchEvent(event);
            if(!this.detector.onTouchEvent(event)) {
                retVal = super.onTouchEvent(event);
            }
        }
        return retVal;
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

