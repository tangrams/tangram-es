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
        
        Tangram tangram = new Tangram(this);
        
		view = new GLSurfaceView(this);
		view.setEGLContextClientVersion(2);
		view.setRenderer(tangram);
		setContentView(view);
		
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

