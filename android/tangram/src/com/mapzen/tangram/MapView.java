package com.mapzen.tangram;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.util.AttributeSet;
import android.widget.FrameLayout;

public class MapView extends FrameLayout {

    protected GLSurfaceView glSurfaceView;

    public MapView(Context context) {

        super(context);

        configureGLSurfaceView();

    }

    public MapView(Context context, AttributeSet attrs) {

        super(context, attrs);

        configureGLSurfaceView();

    }

    public interface OnMapReadyCallback {

        void onMapReady(MapController mapController);

    }

    /**
     * Construct a MapController asynchronously; may only be called from the UI thread
     * @param listener The object to receive the resulting MapController in a callback;
     * the callback will be made on the UI thread
     * @param sceneFilePath Location of the YAML scene file within the asset bundle
     */
    public void getMapAsync(final OnMapReadyCallback listener, final String sceneFilePath) {

        final Context context = getContext();

        new AsyncTask<Void, Void, MapController>() {

            @Override
            protected MapController doInBackground(Void... params) {
                return new MapController(context, sceneFilePath);
            }

            @Override
            protected void onPostExecute(MapController map) {
                map.setView(glSurfaceView);
                addView(glSurfaceView);
                listener.onMapReady(map);
            }

        }.execute();

    }

    protected void configureGLSurfaceView() {

        glSurfaceView = new GLSurfaceView(getContext());
        glSurfaceView.setEGLContextClientVersion(2);
        glSurfaceView.setPreserveEGLContextOnPause(true);
        glSurfaceView.setEGLConfigChooser(new ConfigChooser(8, 8, 8, 0, 16, 0));

    }


}
