package com.mapzen.tangram;

import android.graphics.Bitmap;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.view.View;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MapRenderer  implements GLSurfaceView.Renderer {


    public MapRenderer(MapController mapController, Handler uiThreadHandler) {
        this.uiThreadHandler = uiThreadHandler;

        this.map = mapController;
        this.mapPointer = mapController.mapPointer;
    }
    // GLSurfaceView.Renderer methods
    // ==============================

    @Override
    public void onDrawFrame(final GL10 gl) {
        // MapState constants: View is complete when no other flags are set.
        final int VIEW_COMPLETE =    0;
        final int VIEW_CHANGING =    1 << 0;
        //final int LABELS_CHANGING =  1 << 1;
        //final int TILES_LOADING =    1 << final int SCENE_PENDING =    1 << 4;
        final int VIEW_ANIMATING =   1 << 5;

        final long newTime = System.nanoTime();
        final float delta = (newTime - time) / 1000000000.0f;
        time = newTime;

        if (mapPointer <= 0) {
            // No native instance is initialized, so stop here. This can happen during Activity
            // shutdown when the map has been disposed but the View hasn't been destroyed yet.
            return;
        }

        boolean mapViewComplete;
        boolean isCameraEasing;
        boolean isAnimating;

        synchronized(map) {
            int state = nativeUpdate(mapPointer, delta);

            nativeRender(mapPointer);

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
        if (mapPointer <= 0) {
            // No native instance is initialized, so stop here. This can happen during Activity
            // shutdown when the map has been disposed but the View hasn't been destroyed yet.
            return;
        }

        synchronized (map) {
            nativeResize(mapPointer, width, height);
        }
    }

    @Override
    public void onSurfaceCreated(final GL10 gl, final EGLConfig config) {
        if (mapPointer <= 0) {
            // No native instance is initialized, so stop here. This can happen during Activity
            // shutdown when the map has been disposed but the View hasn't been destroyed yet.
            return;
        }

        synchronized (map) {
            nativeSetupGL(mapPointer);
        }
    }

    public void captureFrame(MapController.FrameCaptureCallback callback, boolean waitForCompleteView) {
        frameCaptureCallback = callback;
        frameCaptureAwaitCompleteView = waitForCompleteView;

    }

    @NonNull
    private Bitmap capture() {
        View view = map.getGLViewHolder().getView();

        final int w = view.getWidth();
        final int h = view.getHeight();

        final int b[] = new int[w * h];
        final int bt[] = new int[w * h];

        synchronized (map) {
            nativeCaptureSnapshot(mapPointer, b);
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
    private final long mapPointer;
    private long time = System.nanoTime();
    private boolean isPrevCameraEasing = false;
    private boolean isPrevMapViewComplete = false;


    private native void nativeSetupGL(long mapPtr);
    private native void nativeResize(long mapPtr, int width, int height);
    private native int nativeUpdate(long mapPtr, float dt);
    private native void nativeRender(long mapPtr);
    private native void nativeCaptureSnapshot(long mapPtr, int[] buffer);

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
