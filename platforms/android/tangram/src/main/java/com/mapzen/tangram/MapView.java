package com.mapzen.tangram;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.widget.FrameLayout;

/**
 * {@code MapView} is a View for displaying a Tangram map.
 */
public class MapView extends FrameLayout {

    protected GLSurfaceView glSurfaceView;
    protected MapController mapController;

    public MapView(@NonNull final Context context) {
        super(context);
        configureGLSurfaceView();
    }

    public MapView(@NonNull final Context context, @Nullable final AttributeSet attrs) {
        super(context, attrs);
        configureGLSurfaceView();
    }

    /**
     * Construct a {@code MapController}; may only be called from the UI thread
     * @param listener The listener to receive to receive scene load events;
     * the callback will be made on the UI thread
     */
    @NonNull
    public MapController getMap(final MapController.SceneLoadListener listener) {
        if (mapController != null) {
            return mapController;
        }
        mapController = getMapInstance();
        mapController.setSceneLoadListener(listener);
        mapController.init();

        return mapController;
    }

    @NonNull
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
    public void onCreate(final Bundle savedInstanceState) {

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
     * Any access to MapView.mapController is illegal after this call.
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
