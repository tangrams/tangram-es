package com.mapzen.tangram.viewholder;

import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.support.annotation.NonNull;
import android.support.annotation.UiThread;
import android.util.Log;
import android.view.TextureView;
import android.view.TextureView.SurfaceTextureListener;
import android.view.View;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

import java.lang.ref.WeakReference;
import java.util.ArrayList;

/*
How TextureView, SurfaceTexture, EGLSurface and gl texture related:
    - Render thread draws with GL on its local EGLSurface on a window surface it created.
    - The window surface is backed by the SurfaceTexture from TextureVIew.
    - So one has to use SurfaceTexture from the TextureView using eglCreateWindowSurface to get the eglSurface.
    - The SurfaceTexture takes what is rendered onto it and makes it available as a GL texture.
    - TextureView takes the GL texture and renders it onto its EGLSurface.
    - This EGLSurface (TextureView) is a window surface visible to the compositor.
 */

/*
 * Unlike GLSurfaceView, TextureView doesn't manage the EGL config or renderer thread, so we
 * take care of that ourselves. TextureViewHolder will do the following:
 * 1. manage communication between ui thread and itself (gl render thread)
 * 2. manage the gl and egl contexts
 */

public class TextureViewHolder implements GLViewHolder, SurfaceTextureListener {

    private final TextureView textureView;
    private final GLThread glThread;
    private Renderer renderer;
    private GLSurfaceView.EGLConfigChooser eglConfigChooser;
    private int eglContextClientVersion;

    private final Object syncGLThread = new Object();

    public TextureViewHolder(@NonNull TextureView textureView) {
        this.textureView = textureView;
        glThread = new GLThread(new WeakReference<>(this));
    }

    // TextureView.SurfaceTextureListener methods
    // ==========================================

    @UiThread
    @Override
    public void onSurfaceTextureAvailable(final SurfaceTexture surface, final int width, final int height) {
        synchronized (syncGLThread) {
            glThread.width = width;
            glThread.height = height;
            glThread.surfaceTexture = surface;
            glThread.requestRender = true;
            syncGLThread.notifyAll();
        }
    }

    @UiThread
    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, final int width, final int height) {
        synchronized (syncGLThread) {
            glThread.width = width;
            glThread.height = height;
            glThread.sizeChanged = true;
            glThread.requestRender = true;
            syncGLThread.notifyAll();
        }
    }

    @UiThread
    @Override
    public boolean onSurfaceTextureDestroyed(final SurfaceTexture surface) {
        synchronized (syncGLThread) {
            glThread.surfaceTexture.release();
            glThread.surfaceTexture = null;
            if (glThread.shouldExit || (glThread.pauseRequest && !glThread.preserveEGLContextOnPause)) {
                glThread.destroyContext = true;
            }
            glThread.requestRender = false;
            syncGLThread.notifyAll();
        }
        return true;
    }

    @UiThread
    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        // No-op
    }


    // GLViewHolder Methods
    // =========================

    @Override
    public void setRenderer(@NonNull Renderer renderer) {
        if (eglConfigChooser == null) {
            eglConfigChooser = new ConfigChooser(8, 8, 8, 0, 16, 0);
        }
        this.renderer = renderer;
        textureView.setSurfaceTextureListener(this);
        glThread.start();
    }

    @Override
    public void setRenderMode(RenderMode renderMode) {
        glThread.renderMode = renderMode;
    }

    @Override
    public RenderMode getRenderMode() {
        return glThread.renderMode;
    }

    @Override
    public void requestRender() {
        synchronized (syncGLThread) {
            glThread.requestRender = true;
            syncGLThread.notifyAll();
        }
    }

    @Override
    public void queueEvent(Runnable runnable) {
        synchronized (syncGLThread) {
            glThread.qEvents.add(runnable);
            syncGLThread.notifyAll();
        }
    }

    @Override
    @NonNull
    public View getView() {
        return textureView;
    }

    void setPreserveEGLContextOnPause(boolean preserveOnPause) {
        glThread.preserveEGLContextOnPause = preserveOnPause;
    }

    void setEGLConfigChooser(GLSurfaceView.EGLConfigChooser configChooser) {
        this.eglConfigChooser = configChooser;
    }

    void setEGLContextClientVersion(int version) {
        this.eglContextClientVersion = version;
    }

    @Override
    public void onPause() {
        this.glThread.onPause();
    }

    @Override
    public void onResume() {
        glThread.onResume();
    }

    @Override
    public void onDestroy() {
        glThread.onDestroy();
        textureView.setSurfaceTextureListener(null);
    }


    // GLThread
    // ========

    private static class GLThread extends Thread {

        private final WeakReference<TextureViewHolder> textureViewHolderWeakReference;
        private final Object syncGLThread;
        private EGLHelper eglHelper;

        private boolean shouldExit;
        private boolean exited;
        private boolean requestRender;
        private boolean sizeChanged;
        private boolean paused = false;
        private boolean pauseRequest = false;
        private boolean destroyContext;
        private boolean destroySurface;
        private int width;
        private int height;

        private boolean preserveEGLContextOnPause = false;
        private RenderMode renderMode = RenderMode.RENDER_CONTINUOUSLY;

        private SurfaceTexture surfaceTexture;
        private final ArrayList<Runnable> qEvents = new ArrayList<>();

        GLThread(WeakReference<TextureViewHolder> viewHolder) {
            this.textureViewHolderWeakReference = viewHolder;
            TextureViewHolder textureViewHolder = viewHolder.get();
            TextureView tv = (TextureView)textureViewHolder.getView();
            eglHelper = new EGLHelper(new WeakReference<>(tv));
            this.syncGLThread = textureViewHolder.syncGLThread;
        }

        @UiThread
        void onPause() {
            synchronized (syncGLThread) {
                this.pauseRequest = true;
                syncGLThread.notifyAll();
            }
        }

        @UiThread
        public void onResume() {
            synchronized (syncGLThread) {
                this.pauseRequest = false;
                syncGLThread.notifyAll();
            }
        }

        @UiThread
        public void onDestroy() {
            synchronized (syncGLThread) {
                this.shouldExit = true;
                syncGLThread.notifyAll();
                while (!this.exited) {
                    try {
                        syncGLThread.wait();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }

        // Overriden thread functions
        @Override
        public void run() {
            setName("MapTextureGLThread " + getId());
            try {
                guardedRun();
            } catch (InterruptedException r) {
            } finally {
                eglHelper.cleanup();
                synchronized (syncGLThread) {
                    this.exited = true;
                    syncGLThread.notifyAll();
                }
            }
        }

        private void guardedRun() throws InterruptedException {

            try {
                while (true) {
                    GL10 gl = null;
                    Runnable event = null;
                    int w = -1;
                    int h = -1;
                    boolean initGLContext = false;
                    boolean initGLSurface = false;
                    boolean pausing = false;

                    // updates states for the thread
                    synchronized (syncGLThread) {
                        while (true) {
                            if (shouldExit) {
                                return;
                            }

                            /*
                             * Wait for a valid SurfaceTexture to be created from TextureView
                             * No GL can happen before this!!
                             */
                            while (surfaceTexture == null) {
                                syncGLThread.wait();
                            }

                            // grab an event to be processed by the gl thread
                            if (!qEvents.isEmpty()) {
                                event = qEvents.remove(0);
                                break;
                            }

                            if (destroySurface) {
                                eglHelper.destroySurface();
                                destroySurface = false;
                                break;
                            }

                            if (destroyContext) {
                                eglHelper.destroyContext();
                                destroyContext = false;
                            }

                            if (paused != pauseRequest) {
                                pausing = pauseRequest;
                                paused = pauseRequest;
                            }

                            // When pausing, release the EGL surface:
                            if (pausing && eglHelper.eglSurface != EGL10.EGL_NO_SURFACE) {
                                eglHelper.destroySurface();
                                destroySurface = false;
                            }

                            // When pausing, optionally release the EGL Context:
                            if (pausing && eglHelper.eglContext != EGL10.EGL_NO_CONTEXT &&
                                    !preserveEGLContextOnPause) {
                                eglHelper.destroyContext();
                            }

                            // Surface is available because of onSurfaceTextureAvailable call
                            // thread is not paused
                            // there is a request to render
                            if (surfaceTexture != null && !paused &&
                                    (requestRender || (renderMode == RenderMode.RENDER_CONTINUOUSLY))) {
                                w = width;
                                h = height;
                                if (eglHelper.eglContext == EGL10.EGL_NO_CONTEXT) {
                                    initGLContext = true;
                                    break;
                                }
                                if (eglHelper.eglSurface == EGL10.EGL_NO_SURFACE) {
                                    // surface must have been destroyed or not created at an earlier attempt
                                    initGLSurface = true;
                                    break;
                                }
                                // reset requestRender for subsequent render request to be set!
                                requestRender = false;
                                break;
                            }
                            syncGLThread.wait();
                        }
                    }

                    TextureViewHolder textureViewHolder = textureViewHolderWeakReference.get();

                    // Because a previous onPause could have destroyed surfaceTexture (TextureView's)
                    if (pausing) {
                        continue;
                    }

                    if (event != null) {
                        event.run();
                        continue;
                    }

                    if (initGLContext) {
                        eglHelper.setupEGL(textureViewHolder.eglConfigChooser, textureViewHolder.eglContextClientVersion);
                        gl = eglHelper.createGL();
                        if (!eglHelper.createSurface()) {
                            synchronized (syncGLThread) {
                                destroySurface = true;
                            }
                            continue;
                        }
                        textureViewHolder.renderer.onSurfaceCreated(gl, eglHelper.eglConfig);
                        textureViewHolder.renderer.onSurfaceChanged(gl, w, h);
                        // make sure `onSurfaceChanged` is not handled again, below
                        sizeChanged = false;
                    }

                    if (initGLSurface) {
                        eglHelper.createSurface();
                        textureViewHolder.renderer.onSurfaceCreated(gl, eglHelper.eglConfig);
                        textureViewHolder.renderer.onSurfaceChanged(gl, w, h);
                        sizeChanged = false;
                    }

                    if (sizeChanged) {
                        textureViewHolder.renderer.onSurfaceChanged(gl, w, h);
                        sizeChanged = false;
                    }

                    // we should have valid eglcontext and eglsurface now proceed with native rendering
                    textureViewHolder.renderer.onDrawFrame(gl);

                    // swap the buffers
                    int error = eglHelper.swap();
                    switch (error) {
                        case EGL10.EGL_SUCCESS:
                            break;
                        case EGL11.EGL_CONTEXT_LOST:
                            synchronized (syncGLThread) {
                                surfaceTexture = null;
                                destroySurface = true;
                                destroyContext = true;
                            }
                            break;
                        default:
                            // Other errors typically mean that the current surface is bad,
                            // probably because the SurfaceView surface has been destroyed,
                            // but we haven't been notified yet.
                            synchronized (syncGLThread) {
                                surfaceTexture = null;
                                destroySurface = true;
                            }
                    }
                }
            } finally {
                synchronized (syncGLThread) {
                    eglHelper.cleanup();
                }
            }

        }
    }

    // Reference explaining EGL handling and opengl es stack in android:
    // https://www.androidcookbook.info/opengl-3d/getting-an-egl-context.html
    // EGL context info (Helper class to encapsulate EGL handling)
    private static class EGLHelper {
        static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

        // Weak reference to the textureView for which this EGL context will be created!
        private final WeakReference<TextureView> textureViewWeakRef;

        private EGL10 egl;
        private EGLConfig eglConfig = null;
        private EGLDisplay eglDisplay = EGL10.EGL_NO_DISPLAY;
        private EGLContext eglContext = EGL10.EGL_NO_CONTEXT;
        private EGLSurface eglSurface = EGL10.EGL_NO_SURFACE;

        EGLHelper(WeakReference<TextureView> textureViewWeakRef) {
            this.textureViewWeakRef = textureViewWeakRef;
        }

        void setupEGL(GLSurfaceView.EGLConfigChooser eglConfigChooser, int eglContextClientVersion) {
            this.egl = (EGL10) EGLContext.getEGL();

            // get display
            this.eglDisplay = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
            if (eglDisplay == EGL10.EGL_NO_DISPLAY) {
                throwEglException("eglGetDisplay");
            }

            //init display
            int[] eglVersion = new int[2];
            if (!egl.eglInitialize(eglDisplay, eglVersion)) {
                throwEglException("eglInitialize");
            }

            //create egl context
            if (textureViewWeakRef == null) {
                eglDisplay = EGL10.EGL_NO_DISPLAY;
            } else if (eglContext == EGL10.EGL_NO_CONTEXT) {
                // Using the same config used for glSurfaceView eglConfigChooser
                eglConfig = eglConfigChooser.chooseConfig(egl, eglDisplay);
                // create opengl es 2.0 context
                int[] attr_list = {EGL_CONTEXT_CLIENT_VERSION, eglContextClientVersion, EGL10.EGL_NONE};
                eglContext = egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attr_list);
            }

            if (eglDisplay == EGL10.EGL_NO_DISPLAY || eglContext == null || eglContext == EGL10.EGL_NO_CONTEXT) {
                eglContext = null;
                throwEglException("eglCreateContext");
            }
        }

        GL10 createGL() {
            return (GL10) eglContext.getGL();
        }

        boolean createSurface() {
            //make sure to destroy the previous surface
            destroySurface();

            // create egl surface to render onto using the surfaceTexture from our textureview
            TextureView textureView = textureViewWeakRef.get();
            if (textureView == null) {
                eglSurface = EGL10.EGL_NO_SURFACE;
            } else {
                eglSurface = egl.eglCreateWindowSurface(eglDisplay, eglConfig, textureView.getSurfaceTexture(), null);
            }

            if (eglSurface == null || eglSurface == EGL10.EGL_NO_SURFACE) {
                Log.e("EGLHelper", eglErrorToString(egl.eglGetError()));
                return false;
            }

            return makeCurrent();
        }

        boolean makeCurrent() {
            if (!egl.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
                Log.w("EGLHelper", eglErrorToString(egl.eglGetError()));
                return false;
            }
            return true;
        }

        // swap buffers
        int swap() {
            if (!egl.eglSwapBuffers(eglDisplay, eglSurface)) {
                return egl.eglGetError();
            }
            return EGL10.EGL_SUCCESS;
        }

        // destroy surface
        private void destroySurface() {
            if (eglSurface != EGL10.EGL_NO_SURFACE && !egl.eglDestroySurface(eglDisplay, eglSurface)) {
                Log.w("EGLHelper", eglErrorToString(egl.eglGetError()));
            }
            eglSurface = EGL10.EGL_NO_SURFACE;
        }

        // destroy context
        private void destroyContext() {
            if (eglContext != EGL10.EGL_NO_CONTEXT && !egl.eglDestroyContext(eglDisplay, eglContext)) {
                Log.w("EGLHelper", eglErrorToString(egl.eglGetError()));
            }
            eglContext = EGL10.EGL_NO_CONTEXT;
        }

        // destroy display
        private void terminate() {
            if (eglDisplay != EGL10.EGL_NO_DISPLAY && !egl.eglTerminate(eglDisplay)) {
                Log.w("EGLHelper", eglErrorToString(egl.eglGetError()));
            }
            eglDisplay = EGL10.EGL_NO_DISPLAY;
        }

        private void throwEglException(String fn) {
            String message = fn + " failed: " + eglErrorToString(egl.eglGetError());
            throw new RuntimeException(message);
        }

        private String eglErrorToString(int error) {
            switch (error) {
                case EGL10.EGL_SUCCESS:
                    return "EGL_SUCCESS";
                case EGL10.EGL_NOT_INITIALIZED:
                    return "EGL_NOT_INITIALIZED";
                case EGL10.EGL_BAD_ACCESS:
                    return "EGL_BAD_ACCESS";
                case EGL10.EGL_BAD_ALLOC:
                    return "EGL_BAD_ALLOC";
                case EGL10.EGL_BAD_ATTRIBUTE:
                    return "EGL_BAD_ATTRIBUTE";
                case EGL10.EGL_BAD_CONFIG:
                    return "EGL_BAD_CONFIG";
                case EGL10.EGL_BAD_CONTEXT:
                    return "EGL_BAD_CONTEXT";
                case EGL10.EGL_BAD_CURRENT_SURFACE:
                    return "EGL_BAD_CURRENT_SURFACE";
                case EGL10.EGL_BAD_DISPLAY:
                    return "EGL_BAD_DISPLAY";
                case EGL10.EGL_BAD_MATCH:
                    return "EGL_BAD_MATCH";
                case EGL10.EGL_BAD_NATIVE_PIXMAP:
                    return "EGL_BAD_NATIVE_PIXMAP";
                case EGL10.EGL_BAD_NATIVE_WINDOW:
                    return "EGL_BAD_NATIVE_WINDOW";
                case EGL10.EGL_BAD_PARAMETER:
                    return "EGL_BAD_PARAMETER";
                case EGL10.EGL_BAD_SURFACE:
                    return "EGL_BAD_SURFACE";
                case EGL11.EGL_CONTEXT_LOST:
                    return "EGL10.EGL_CONTEXT_LOST";
                default:
                    return "0x" + Integer.toHexString(error);
            }
        }

        void cleanup() {
            destroySurface();
            destroyContext();
            terminate();
        }
    }
}
