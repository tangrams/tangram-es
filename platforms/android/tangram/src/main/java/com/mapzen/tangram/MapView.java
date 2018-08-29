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
        void onMapReady(@Nullable MapController mapController);
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

        MapAsyncTask(@NonNull final MapView mapView, @NonNull final MapReadyCallback callback, @Nullable final HttpHandler handler) {
            mapViewRef = new WeakReference<>(mapView);
            httpHandler = handler;
            mapReadyCallback = callback;
        }

        @Override
        protected MapController doInBackground(Void... voids) {
            MapView view = mapViewRef.get();
            if (view == null) {
                return null;
            }
            return view.mapInitInBackground(httpHandler);
        }

        @Override
        protected void onPostExecute(MapController controller) {
            MapView view = mapViewRef.get();
            if (view != null) {
                view.onMapInitOnUIThread(controller, mapReadyCallback);
            }
        }

        @Override
        protected void onCancelled(MapController controller) {
            MapView view = mapViewRef.get();
            if (view != null) {
                view.onMapInitCancelled(controller);
            }
        }
    }

    /**
     * Responsible for doing the actual map init. Should be executed from a non-ui thread.
     * @param handler {@link HttpHandler} required for network handling
     * @return new or previously initialized {@link MapController}
     */
    protected MapController mapInitInBackground(@Nullable final HttpHandler handler) {
        synchronized (lock) {
            if (!NativeLibraryLoader.sNativeLibraryLoaded) {
                android.util.Log.e("Tangram", "Unable to initialize MapController, because failed to initialize native libraries");
                return null;
            }
            if (mapController != null) {
                return mapController;
            }
        }
        return getMapInstance(handler);
    }

    /**
     * Should be executed from the UI thread
     * @param controller {@link MapController} created in a background thread
     * @param callback {@link MapReadyCallback}
     */
    protected void onMapInitOnUIThread(@Nullable final MapController controller, @NonNull final MapReadyCallback callback) {
        synchronized (lock) {
            if (mapController == null && controller != null) {
                mapController = controller;
                configureGLSurfaceView();
                mapController.UIThreadInit();
            }
            callback.onMapReady(mapController);
        }
    }

    /**
     * To be executed by the async task implementation during cancellation of a mapInit task
     * @param controller {@link MapController} created by a background task which is no longer required
     */
    protected void onMapInitCancelled(@Nullable final MapController controller) {
        if (controller != null) {
            controller.dispose();
        }
    }

    /**
     * Responsible for creating and executing an async task to initialize the Map
     * Note: Can be overridden to support other ways of implementing async tasks (e.g. when using observable with RxJava)
     * @param callback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * @param handler Set the client implemented {@link HttpHandler} for retrieving remote map resources
     *                when null {@link DefaultHttpHandler} is used
     */
    protected void executeMapAsyncTask(@NonNull final MapReadyCallback callback, @Nullable final HttpHandler handler) {
        getMapAsyncTask = new MapAsyncTask(this, callback, handler);
        getMapAsyncTask.execute();
    }

    /**
     * Responsible to dispose any prior running getMapReadyTask
     * Note: Can be overriden to support other ways of implementing async tasks (e.g. when using observable with RxJava)
     */
    protected void disposeMapReadyTask() {
        if (getMapAsyncTask != null) {
            getMapAsyncTask.cancel(true);
        }
        getMapAsyncTask = null;
    }

    /**
     * Construct a {@code MapController} in an async thread; may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * the callback will be made on the UI thread
     */
    public void getMapAsync(@NonNull final MapReadyCallback readyCallback) {
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
    public void getMapAsync(@NonNull final MapReadyCallback readyCallback,
                            @Nullable final HttpHandler handler) {

        disposeMapReadyTask();
        if (glSurfaceView == null) {
            glSurfaceView = new GLSurfaceView(getContext());
        }
        executeMapAsyncTask(readyCallback, handler);
    }

    @NonNull
    protected MapController getMapInstance(@Nullable final HttpHandler handler) {
        return new MapController(glSurfaceView, handler);
    }

    protected void configureGLSurfaceView() {
        glSurfaceView.setEGLContextClientVersion(2);
        glSurfaceView.setPreserveEGLContextOnPause(true);
        glSurfaceView.setEGLConfigChooser(new ConfigChooser(8, 8, 8, 0, 16, 8));
        addView(glSurfaceView);
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
