package com.mapzen.tangram.viewholder;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.support.annotation.NonNull;

public class GLSurfaceViewHolderFactory implements GLViewHolderFactory {

    /**
     * Responsible to create an instance of {@link GLSurfaceViewHolder} which holds
     * an instance of {@link GLSurfaceView} for map display.
     * @param context Application Context
     * @return {@link GLSurfaceViewHolder}
     */
    @Override
    @NonNull
    public GLViewHolder build(Context context) {
        GLSurfaceView glSurfaceView = new GLSurfaceView(context);
        return new GLSurfaceViewHolder(glSurfaceView);
    }
}
