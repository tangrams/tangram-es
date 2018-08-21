package com.mapzen.tangram;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import com.mapzen.tangram.networking.DefaultHttpHandler;
import com.mapzen.tangram.networking.HttpHandler;

/**
 * {@code MapView} is a View for displaying a Tangram map.
 */
public class MapView extends FrameLayout {

    protected GLSurfaceView glSurfaceView;
    protected MapController mapController;
    protected AsyncTask<Void, Void, MapController> getMapAsyncTask;

    /**
     * MapReadyCallback interface
     * onMapReady gets invoked on the ui thread when {@link MapController} is instantiated and ready to be used
     */
    public interface MapReadyCallback {
        void onMapReady(MapController mapController);
    }

    public MapView(@NonNull final Context context) {
        super(context);
        configureGLSurfaceView();
    }

    public MapView(@NonNull final Context context, @Nullable final AttributeSet attrs) {
        super(context, attrs);
        configureGLSurfaceView();
    }

    /**
     * Construct a {@code MapController} in an async thread; may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     */
    public void getMapAsync(@Nullable final MapReadyCallback readyCallback) {
        getMapAsync(readyCallback, null, null);
    }

    /**
     * Construct a {@code MapController} in an async thread; may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * @param sceneLoadListener The listener to receive to receive scene load events;
     * the callback will be made on the UI thread
     */
    public void getMapAsync(@Nullable final MapReadyCallback readyCallback,
                            @Nullable final MapController.SceneLoadListener sceneLoadListener) {
        getMapAsync(readyCallback, sceneLoadListener, null);
    }

    /**
     * Construct a {@code MapController} in an async thread; may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * @param sceneLoadListener The listener to receive to receive scene load events;
     * @param handler Set the client implemented {@link HttpHandler} for retrieving remote map resources
     *                when null {@link DefaultHttpHandler} is used
     */
    public void getMapAsync(@Nullable final MapReadyCallback readyCallback,
                            @Nullable final MapController.SceneLoadListener sceneLoadListener,
                            @Nullable final HttpHandler handler) {

        disposeMapReadyTask();

        final Context ctx = glSurfaceView.getContext();

        getMapAsyncTask = new AsyncTask<Void, Void, MapController>() {
            @Override
            protected MapController doInBackground(Void... voids) {
                if (mapController != null) {
                    return mapController;
                }
                MapController controller = getMapInstance();
                controller.init(ctx, handler);

                return controller;
            }

            @Override
            protected void onPostExecute(MapController controller) {
                if (mapController != null) {
                    mapController.dispose();
                }
                mapController = controller;
                mapController.postInit();
                mapController.setSceneLoadListener(sceneLoadListener);
                readyCallback.onMapReady(mapController);
            }

            @Override
            protected void onCancelled(MapController controller) {
                if (controller != null) {
                    controller.dispose();
                }
            }
        }.execute();

    }

    /**
     * Construct a {@code MapController}; may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param listener The listener to receive to receive scene load events;
     * the callback will be made on the UI thread
     */
    @NonNull
    public MapController getMap(@Nullable final MapController.SceneLoadListener listener) {
        return getMap(listener, null);
    }

    /**
     * Construct a {@code MapController}; may only be called from the UI thread
     * @param listener The listener to receive to receive scene load events;
     * @param handler Set the client implemented {@link HttpHandler} for retrieving remote map resources
     *                when null {@link DefaultHttpHandler} is used
     * the callback will be made on the UI thread
     */
    @NonNull
    public MapController getMap(@Nullable final MapController.SceneLoadListener listener, @Nullable final HttpHandler handler) {
        disposeMapReadyTask();
        if (mapController != null) {
            return mapController;
        }
        mapController = getMapInstance();

        mapController.setSceneLoadListener(listener);
        mapController.init(glSurfaceView.getContext(), handler);
        mapController.postInit();

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

    /**
     * Responsible to dispose any getMapReadyTask
     */
    protected void disposeMapReadyTask() {
        if (getMapAsyncTask != null) {
            getMapAsyncTask.cancel(true);
        }
        getMapAsyncTask = null;
    }

    protected void disposeMap() {
        disposeMapReadyTask();

        if (mapController != null) {
            // MapController has been initialized, so we'll dispose it now.
            mapController.dispose();
            mapController = null;
        }
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
