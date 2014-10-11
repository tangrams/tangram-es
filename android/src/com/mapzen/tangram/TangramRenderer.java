package com.mapzen.tangram;

import android.opengl.GLSurfaceView;
import android.opengl.GLES10;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;

public class TangramRenderer implements GLSurfaceView.Renderer {
	
	static {
		System.loadLibrary("c++_shared");
		System.loadLibrary("tangram");
	}

	private static native void init();
	private static native void resize(int width, int height);
	private static native void render();

	public void onDrawFrame(GL10 gl) 
	{
		render();
	}

	public void onSurfaceChanged(GL10 gl, int width, int height) 
	{
		resize(width, height);
	}

	public void onSurfaceCreated(GL10 gl, EGLConfig config) 
	{
		init();
	}

}