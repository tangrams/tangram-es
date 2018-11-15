package com.mapzen.tangram;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.FrameLayout;

import com.mapzen.tangram.viewholder.GLSurfaceViewHolderFactory;
import com.mapzen.tangram.viewholder.GLViewHolder;
import com.mapzen.tangram.viewholder.GLViewHolderFactory;
import com.mapzen.tangram.networking.DefaultHttpHandler;
import com.mapzen.tangram.networking.HttpHandler;

import java.lang.ref.WeakReference;

/**
 * {@code MapView} is a View for displaying a Tangram map.
 */
public class MapView extends FrameLayout {

    protected MapController mapController;
    protected GLViewHolder viewHolder;
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
        GLViewHolderFactory glViewHolderFactory;
        HttpHandler httpHandler;

        WeakReference<MapView> mapViewRef;

        MapAsyncTask(@NonNull final MapView mapView, @NonNull final MapReadyCallback callback,
                     @NonNull final GLViewHolderFactory viewHolderFactory, @Nullable final HttpHandler handler) {
            mapViewRef = new WeakReference<>(mapView);
            httpHandler = handler;
            mapReadyCallback = callback;
            glViewHolderFactory = viewHolderFactory;
        }

        @Override
        protected MapController doInBackground(Void... voids) {
            MapView view = mapViewRef.get();
            if (view != null) {
                return view.mapInitInBackground();
            }
            return null;
        }

        @Override
        protected void onPostExecute(MapController controller) {
            MapView view = mapViewRef.get();
            if (view != null) {
                view.onMapInitOnUIThread(controller, httpHandler, glViewHolderFactory, mapReadyCallback);
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
                                       @NonNull GLViewHolderFactory viewHolderFactory, @NonNull final MapReadyCallback callback) {
        if (viewHolder != null) {
            removeView(viewHolder.getView());
            viewHolder = null;
        }

        if (mapController != null) {
            mapController.dispose();
            mapController = null;
        }

        if (controller != null) {
            viewHolder = viewHolderFactory.build(getContext());
            if (viewHolder == null) {
                android.util.Log.e("Tangram", "Unable to initialize MapController: Failed to initialize OpenGL view");
                controller.dispose();
            } else {
                mapController = controller;
                mapController.UIThreadInit(viewHolder, handler);
                addView(viewHolder.getView());
                Log.d("Tangram", "UIThreadInit " + getWidth() + " / "+ getHeight());
                mapController.resize( getWidth(), getHeight());
            }
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
    protected void executeMapAsyncTask(@NonNull final MapReadyCallback callback,
                                       @NonNull GLViewHolderFactory viewHolderFactory,
                                       @Nullable final HttpHandler handler) {
        getMapAsyncTask = new MapAsyncTask(this, callback, viewHolderFactory, handler);
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
     * Construct a {@code MapController} in an async thread using {@link GLSurfaceView} as GL rendering interface;
     * may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     */
    public void getMapAsync(@NonNull final MapReadyCallback readyCallback) {
        getMapAsync(readyCallback, new GLSurfaceViewHolderFactory(), null);
    }

    /**
     * Construct a {@code MapController} in an async thread using {@link GLSurfaceView} as GL rendering interface;
     * may only be called from the UI thread
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * @param handler Set the client implemented {@link HttpHandler} for retrieving remote map resources
     *                when null {@link DefaultHttpHandler} is used
     */
    public void getMapAsync(@NonNull final MapReadyCallback readyCallback,
                            @Nullable final HttpHandler handler) {
        getMapAsync(readyCallback, new GLSurfaceViewHolderFactory(), handler);
    }

    /**
     * Construct a {@code MapController} in an async thread using an instance
     * {@link GLViewHolderFactory}, like {@link GLSurfaceViewHolderFactory} or any client implementation
     * may only be called from the UI thread
     * Map instance uses {@link DefaultHttpHandler} for retrieving remote map resources
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * @param glViewHolderFactory An instance of {@link GLViewHolderFactory} providing the GL responsibilities
     */
    public void getMapAsync(@NonNull final MapReadyCallback readyCallback,
                            @NonNull final GLViewHolderFactory glViewHolderFactory) {
        getMapAsync(readyCallback, glViewHolderFactory, null);
    }

    /**
     * Construct a {@code MapController} in an async thread using an instance
     * {@link GLViewHolderFactory}, like {@link GLSurfaceViewHolderFactory} or any client implementation
     * may only be called from the UI thread
     * @param readyCallback {@link MapReadyCallback#onMapReady(MapController)} to be invoked when
     * {@link MapController} is instantiated and ready to be used. The callback will be made on the UI thread
     * @param glViewHolderFactory An instance of {@link GLViewHolderFactory} providing the GL responsibilities
     * @param handler Set the client implemented {@link HttpHandler} for retrieving remote map resources
     *                when null {@link DefaultHttpHandler} is used
     */
    public void getMapAsync(@NonNull final MapReadyCallback readyCallback,
                            GLViewHolderFactory glViewHolderFactory,
                            @Nullable final HttpHandler handler) {
        disposeMapReadyTask();
        executeMapAsyncTask(readyCallback, glViewHolderFactory, handler);
    }

    protected void disposeMap() {
        disposeMapReadyTask();

        if (viewHolder != null) {
            viewHolder.onPause();
            viewHolder = null;
        }

        if (mapController != null) {
            mapController.dispose();
            mapController = null;
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        return mapController.handleGesture(this, ev);
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
        if (viewHolder != null) {
            viewHolder.onResume();
        }
    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     */
    public void onPause() {
        if (viewHolder != null) {
            viewHolder.onPause();
        }
    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     * Any access to MapView.mapController is illegal after this call.
     */
    public void onDestroy() {
        if (viewHolder != null) {
            viewHolder.onDestroy();
        }
        disposeMap();
    }

    /**
     * You must call this method from the parent Activity/Fragment's corresponding method.
     */
    public void onLowMemory() {
        mapController.onLowMemory();
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        Log.d("Tangram", "WINDOW LAYOUT " + getWidth() + " / "+ getHeight());
        if (mapController != null) {
            mapController.resize( getWidth(), getHeight());
        }
    }

}
