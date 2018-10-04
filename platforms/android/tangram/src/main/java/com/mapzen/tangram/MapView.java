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
            return view.mapInitInBackground();
        }

        @Override
        protected void onPostExecute(MapController controller) {
            MapView view = mapViewRef.get();
            if (view != null) {
                view.onMapInitOnUIThread(controller, httpHandler, mapReadyCallback);
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
     * @return new or previously initialized {@link MapController}
     */
    protected MapController mapInitInBackground() {

        if (!NativeLibraryLoader.sNativeLibraryLoaded) {
            android.util.Log.e("Tangram", "Unable to initialize MapController: Failed to initialize native libraries");
            return null;
        }

        return getMapInstance();
    }

    /**
     * Should be executed from the UI thread
     * @param controller {@link MapController} created in a background thread
     * @param handler {@link HttpHandler} required for network handling
     * @param callback {@link MapReadyCallback}
     */
    protected void onMapInitOnUIThread(@Nullable final MapController controller, @Nullable final HttpHandler handler,
                                       @NonNull final MapReadyCallback callback) {
        if (glSurfaceView != null) {
            removeView(glSurfaceView);
            glSurfaceView = null;
        }

        if (mapController != null) {
            mapController.dispose();
            mapController = null;
        }

        if (controller != null) {
            mapController = controller;
            glSurfaceView = createGLSurfaceView();
            mapController.UIThreadInit(glSurfaceView, handler);
            addView(glSurfaceView);
        }
        callback.onMapReady(mapController);
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

        executeMapAsyncTask(readyCallback, handler);
    }

    protected GLSurfaceView createGLSurfaceView() {
        GLSurfaceView view = new GLSurfaceView(getContext());
        view.setEGLContextClientVersion(2);
        view.setPreserveEGLContextOnPause(true);
        view.setEGLConfigChooser(new ConfigChooser(8, 8, 8, 0, 16, 8));
        return view;
    }

    protected void disposeMap() {
        disposeMapReadyTask();

        if (glSurfaceView != null) {
            glSurfaceView.onPause();
            glSurfaceView = null;
        }

        if (mapController != null) {
            mapController.dispose();
            mapController = null;
        }
    }

    protected MapController getMapInstance() {
        return new MapController(this.getContext());
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
        if (glSurfaceView != null) {
            glSurfaceView.onResume();
        }
    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     */
    public void onPause() {
        if (glSurfaceView != null) {
            glSurfaceView.onPause();
        }
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
