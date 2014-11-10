package com.mapzen.tangram;

import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLES10;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;

public class TangramRenderer implements GLSurfaceView.Renderer {
	
	static {
		System.loadLibrary("c++_shared");
		System.loadLibrary("tangram");
	}

	private static native void init(AssetManager assetManager);
	private static native void resize(int width, int height);
	private static native void update(float dt);
	private static native void render();

	private long time = System.nanoTime();

	private AssetManager assetManager;

	public TangramRenderer(Context mainApp) {
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

	public void onSurfaceChanged(GL10 gl, int width, int height) 
	{
		resize(width, height);
	}

	public void onSurfaceCreated(GL10 gl, EGLConfig config) 
	{
		init(assetManager);
	}

}
