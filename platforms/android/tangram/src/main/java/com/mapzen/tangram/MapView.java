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

import java.lang.ref.WeakReference;

/**
 * {@code MapView} is a View for displaying a Tangram map.
 */
public class MapView extends FrameLayout {

    protected GLSurfaceView glSurfaceView;
    protected MapController mapController;
    protected MapAsyncTask getMapAsyncTask;

    private final Object lock = new Object();

    /**
     * MapReadyCallback interface
     * onMapReady gets invoked on the ui thread when {@link MapController} is instantiated and ready to be used
     */
    public interface MapReadyCallback {
        void onMapReady(MapController mapController);
    }

    public MapView(@NonNull final Context context) {
        super(context);
    }

    public MapView(@NonNull final Context context, @Nullable final AttributeSet attrs) {
        super(context, attrs);
    }

    private static class MapAsyncTask extends AsyncTask<Void, Void, MapController> {
        MapReadyCallback mapReadyCallback;
        HttpHandler httpHandler;

        WeakReference<MapView> mapViewRef;
        WeakReference<Context> contextRef;

        MapAsyncTask(@NonNull final MapView mapView, @Nullable final MapReadyCallback callback, @Nullable final HttpHandler handler) {
            mapViewRef = new WeakReference<>(mapView);
            contextRef = new WeakReference<>(mapView.glSurfaceView.getContext());
            httpHandler = handler;
            mapReadyCallback = callback;
        }

        @Override
        protected MapController doInBackground(Void... voids) {
            MapView view = mapViewRef.get();
            Context ctx = contextRef.get();
            synchronized (view.lock) {
                if (view.mapController != null) {
                    return view.mapController;
                }
            }
            return view.getMapInstance(ctx, httpHandler);
        }

        @Override
        protected void onPostExecute(MapController controller) {
            MapView view = mapViewRef.get();
            synchronized (view.lock) {
                if (view.mapController == null) {
                    view.mapController = controller;
                    view.configureGLSurfaceView();
                    view.mapController.postInit();
                }
                mapReadyCallback.onMapReady(view.mapController);
            }
        }

        @Override
        protected void onCancelled(MapController controller) {
            if (controller != null) {
                controller.dispose();
            }
        }
    }

    /**
     * Construct a {@code MapController} in an async thread; may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * the callback will be made on the UI thread
     */
    public void getMapAsync(@Nullable final MapReadyCallback readyCallback) {
        getMapAsync(readyCallback, null);
    }

    /**
     * Construct a {@code MapController} in an async thread; may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * @param handler Set the client implemented {@link HttpHandler} for retrieving remote map resources
     *                when null {@link DefaultHttpHandler} is used
     */
    public void getMapAsync(@Nullable final MapReadyCallback readyCallback,
                            @Nullable final HttpHandler handler) {

        disposeMapReadyTask();
        if (glSurfaceView == null) {
            glSurfaceView = new GLSurfaceView(getContext());
        }
        getMapAsyncTask = new MapAsyncTask(this, readyCallback, handler);
        getMapAsyncTask.execute();
    }

    @NonNull
    protected MapController getMapInstance(@NonNull final Context context, @Nullable final HttpHandler handler) {
        return new MapController(glSurfaceView, context, handler);
    }

    protected void configureGLSurfaceView() {
        glSurfaceView.setEGLContextClientVersion(2);
        glSurfaceView.setPreserveEGLContextOnPause(true);
        glSurfaceView.setEGLConfigChooser(new ConfigChooser(8, 8, 8, 0, 16, 8));
        addView(glSurfaceView);
    }

    /**
     * Responsible to dispose any prior running getMapReadyTask
     */
    protected void disposeMapReadyTask() {
        if (getMapAsyncTask != null) {
            getMapAsyncTask.cancel(true);
        }
        getMapAsyncTask = null;
    }

    protected void disposeMap() {
        disposeMapReadyTask();

        synchronized (lock) {
            if (mapController != null) {
                // MapController has been initialized, so we'll dispose it now.
                mapController.dispose();
                mapController = null;
            }
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
        synchronized (lock) {
            mapController.onLowMemory();
        }
    }

}
