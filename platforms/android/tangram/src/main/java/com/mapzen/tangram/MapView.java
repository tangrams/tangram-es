package com.mapzen.tangram;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import java.util.ArrayList;
import java.util.List;

/**
 * {@code MapView} is a View for displaying a Tangram map.
 */
public class MapView extends FrameLayout {

    protected GLSurfaceView glSurfaceView;
    protected MapController mapController;

    public MapView(Context context) {
        super(context);
        configureGLSurfaceView();
    }

    public MapView(Context context, AttributeSet attrs) {
        super(context, attrs);
        configureGLSurfaceView();
    }

    /**
     * Construct a {@code MapController}; may only be called from the UI thread
     * @param listener The listener to receive to receive scene load events;
     * the callback will be made on the UI thread
     */
    public MapController getMap(MapController.SceneLoadListener listener) {
        if (mapController != null) {
            return mapController;
        }
        mapController = getMapInstance();
        mapController.setSceneLoadListener(listener);
        mapController.init();

        return mapController;
    }

    protected MapController getMapInstance() {
        return new MapController(glSurfaceView);
    }

    protected void configureGLSurfaceView() {

        glSurfaceView = new GLSurfaceView(getContext());
        glSurfaceView.setEGLContextClientVersion(2);
        glSurfaceView.setPreserveEGLContextOnPause(true);
        glSurfaceView.setEGLConfigChooser(new ConfigChooser(8, 8, 8, 0, 16, 8));
        addView(glSurfaceView);
    }

    protected void disposeMap() {

        if (mapController != null) {
            // MapController has been initialized, so we'll dispose it now.
            mapController.dispose();
        }
        mapController = null;

    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     */
    public void onCreate(Bundle savedInstanceState) {

    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     */
    public void onResume() {

    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     */
    public void onPause() {

    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     */
    public void onDestroy() {

        disposeMap();

    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     */
    public void onLowMemory() {
        mapController.onLowMemory();
    }

}
