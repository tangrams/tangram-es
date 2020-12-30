package com.mapzen.tangram;

import android.graphics.Bitmap;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.view.View;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import androidx.annotation.NonNull;

class MapRenderer implements GLSurfaceView.Renderer {

    MapRenderer(MapController mapController, Handler uiThreadHandler) {
        this.uiThreadHandler = uiThreadHandler;
        this.map = mapController;
        this.nativeMap = mapController.nativeMap;
    }

    // GLSurfaceView.Renderer methods
    // ==============================

    @Override
    public void onDrawFrame(final GL10 gl) {
        // MapState constants: View is complete when no other flags are set.
        final int VIEW_COMPLETE =    0;
        final int VIEW_CHANGING =    1;
        //final int LABELS_CHANGING =  1 << 1;
        //final int TILES_LOADING =    1 << final int SCENE_PENDING =    1 << 4;
        final int VIEW_ANIMATING =   1 << 5;

        final long newTime = System.nanoTime();
        final float delta = (newTime - time) / 1000000000.0f;
        time = newTime;

        boolean mapViewComplete;
        boolean isCameraEasing;
        boolean isAnimating;

        synchronized(map) {
            int state = nativeMap.update(delta);

            nativeMap.render();

            mapViewComplete = (state == VIEW_COMPLETE);
            isCameraEasing = (state & VIEW_CHANGING) != 0;
            isAnimating = (state & VIEW_ANIMATING) != 0;
        }

        if (isCameraEasing) {
            if (!isPrevCameraEasing) {
                uiThreadHandler.post(setMapRegionAnimatingRunnable);
            }
        } else if (isPrevCameraEasing) {
            uiThreadHandler.post(setMapRegionIdleRunnable);
        }

        if (isAnimating) {
            map.requestRender();
        }

        boolean viewCompleted = mapViewComplete && !isPrevMapViewComplete;

        if (viewCompleted && map.mapChangeListener != null) {
            uiThreadHandler.post(viewCompleteRunnable);
        }

        if (frameCaptureCallback != null) {
            final MapController.FrameCaptureCallback cb = frameCaptureCallback;
            frameCaptureCallback = null;

            if (!frameCaptureAwaitCompleteView || viewCompleted) {
                final Bitmap screenshot = capture();
                uiThreadHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        cb.onCaptured(screenshot);
                    }
                });
            }
        }

        isPrevCameraEasing = isCameraEasing;
        isPrevMapViewComplete = mapViewComplete;
    }

    @Override
    public void onSurfaceChanged(final GL10 gl, final int width, final int height) {
        synchronized (map) {
            nativeMap.resize(width, height);
        }
    }

    @Override
    public void onSurfaceCreated(final GL10 gl, final EGLConfig config) {
        synchronized (map) {
            nativeMap.setupGL();
        }
    }

    void captureFrame(MapController.FrameCaptureCallback callback, boolean waitForCompleteView) {
        frameCaptureCallback = callback;
        frameCaptureAwaitCompleteView = waitForCompleteView;
    }

    @NonNull
    private Bitmap capture() {
        GLViewHolder viewHolder = map.getGLViewHolder();
        if (viewHolder == null) {
            throw new IllegalStateException("MapController GLViewHolder is null");
        }

        View view = viewHolder.getView();

        final int w = view.getWidth();
        final int h = view.getHeight();

        final int[] b = new int[w * h];
        final int[] bt = new int[w * h];

        synchronized (map) {
            nativeMap.captureSnapshot(b);
        }
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                final int pix = b[i * w + j];
                final int pb = (pix >> 16) & 0xff;
                final int pr = (pix << 16) & 0x00ff0000;
                final int pix1 = (pix & 0xff00ff00) | pr | pb;
                bt[(h - i - 1) * w + j] = pix1;
            }
        }

        return Bitmap.createBitmap(bt, w, h, Bitmap.Config.ARGB_8888);
    }

    private final Handler uiThreadHandler;
    private final MapController map;
    private final NativeMap nativeMap;
    private long time = System.nanoTime();
    private boolean isPrevCameraEasing = false;
    private boolean isPrevMapViewComplete = false;

    private MapController.FrameCaptureCallback frameCaptureCallback;
    private boolean frameCaptureAwaitCompleteView;

    private final Runnable setMapRegionAnimatingRunnable = new Runnable() {
        @Override
        public void run() {
            map.setMapRegionState(MapController.MapRegionChangeState.ANIMATING);
        }
    };
    private final Runnable setMapRegionIdleRunnable = new Runnable() {
        @Override
        public void run() {
            map.setMapRegionState(MapController.MapRegionChangeState.IDLE);
        }
    };
    private final Runnable viewCompleteRunnable = new Runnable() {
        @Override
        public void run() {
            if (map.mapChangeListener != null) {
                map.mapChangeListener.onViewComplete();
            }
        }
    };
}
