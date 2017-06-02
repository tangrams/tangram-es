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

    public MapController getMap(MapController.SceneLoadListener listener) {
        if (mapController != null) {
            return mapController;
        }
        mapController = getMapInstance();
        mapController.setSceneLoadListener(listener);
        mapController.init();

        return mapController;
    }

    /**
     * Interface for receiving a {@code MapController} once it is ready to be used
     */
    public interface OnMapReadyCallback {

        /**
         * Called when the map is ready to be used
         * @param mapController A non-null {@code MapController} instance for this {@code MapView}
         */
        void onMapReady(MapController mapController);

    }

    /**
     * Construct a {@code MapController} asynchronously; may only be called from the UI thread
     * @param callback The object to receive the resulting MapController in a callback;
     * the callback will be made on the UI thread
     * @param sceneFilePath Location of the YAML scene file within the asset bundle
     */
    public void getMapAsync(@NonNull final OnMapReadyCallback callback,
                            @NonNull final String sceneFilePath) {

        getMapAsync(callback, sceneFilePath, null);
    }

    /**
     * Construct a {@code MapController} asynchronously; may only be called from the UI thread
     * @param callback The object to receive the resulting MapController in a callback;
     * the callback will be made on the UI thread
     * @param sceneFilePath Location of the YAML scene file within the asset bundle
     * @param sceneUpdates List of SceneUpdate to be applied when loading this scene
     */
    public void getMapAsync(@NonNull final OnMapReadyCallback callback,
                            @NonNull final String sceneFilePath,
                            final List<SceneUpdate> sceneUpdates) {

        if (mapController != null) {
            throw new RuntimeException("MapController already initialized");
        }

        mapController = getMapInstance();
        mapController.setMapReadyCallback(callback);
        mapController.init();
        mapController.loadSceneFile(sceneFilePath, sceneUpdates);

    }

    protected MapController getMapInstance() {
        return new MapController(glSurfaceView);
    }

    protected void configureGLSurfaceView() {

        glSurfaceView = new GLSurfaceView(getContext());
        glSurfaceView.setEGLContextClientVersion(2);
        glSurfaceView.setPreserveEGLContextOnPause(true);
        glSurfaceView.setEGLConfigChooser(new ConfigChooser(8, 8, 8, 0, 16, 0));
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
