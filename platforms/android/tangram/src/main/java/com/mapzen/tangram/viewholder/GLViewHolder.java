package com.mapzen.tangram.viewholder;

import android.opengl.GLSurfaceView;
import android.support.annotation.NonNull;
import android.view.View;


/**
 * Interface which holds a GL View, used to display Tangram Map
 */
public interface GLViewHolder {

    enum RenderMode {
        RENDER_WHEN_DIRTY,
        RENDER_CONTINUOUSLY
    };

    /**
     * Set the renderer associated with this view. Also starts the thread that
     * will call the renderer, which in turn causes the rendering to start.
     * @param renderer the renderer to use to perform OpenGL drawing.
     */
    void setRenderer(GLSurfaceView.Renderer renderer);

    /**
     * Control whether the EGL context is preserved when the GLSurfaceView is paused and
     * resumed.
     * @param preserveOnPause preserve the EGL context when paused
     */
    void setPreserveEGLContextOnPause(boolean preserveOnPause);

    /**
     * @return true if the EGL context will be preserved when paused
     */
    boolean getPreserveEGLContextOnPause();

    /**
     * Set the rendering mode. When {@link RenderMode} is
     * RENDER_CONTINUOUSLY, the renderer is called
     * repeatedly to re-render the scene. When {@link RenderMode}
     * is RENDER_WHEN_DIRTY, the renderer only rendered when the surface
     * is created, or when {@link #requestRender} is called. Defaults to RENDER_CONTINUOUSLY.
     * @param renderMode one of the {@link RenderMode} enums
     */
    void setRenderMode(RenderMode renderMode);

    /**
     * Get the current rendering mode.
     * @return the current rendering mode.
     */
    RenderMode getRenderMode();

    /**
     * Install a custom EGLConfigChooser.
     * @param configChooser
     */
    void setEGLConfigChooser(GLSurfaceView.EGLConfigChooser configChooser);

    /**
     * Inform the EGLContext and EGLConfigChooser
     * which EGLContext client version to pick.
     */
    void setEGLContextClientVersion(int version);

    /**
     * Request that the renderer render a frame.
     */
    void requestRender();

    /**
     * Queue a runnable to be run on the GL rendering thread.
     * @param r the runnable to be run on the GL rendering thread.
     */
    void queueEvent(@NonNull final Runnable r);

    /**
     * Pause the rendering thread, optionally tearing down the EGL context
     * depending upon the value of preserveEGLContextOnPause.
     */
    void onPause();

    /**
     * Resumes the rendering thread, re-creating the OpenGL context if necessary.
     */
    void onResume();

    /**
     * Destroys the GL Thread associated with the GLViewHolder
     */
    void onDestroy();

    /**
     * Can be used by the client to get access to the underlying view to pass any view controls
     * @return {@link GLSurfaceView} or Client provided implementation for a view held by {@link GLViewHolder}
     */
    @NonNull
    View getView();

}
