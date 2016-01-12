package com.mapzen.tangram;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

public class MapView extends GLSurfaceView {

    public MapView(Context context) {

        super(context);

        configureGLSurfaceView();

    }

    public MapView(Context context, AttributeSet attrs) {

        super(context, attrs);

        configureGLSurfaceView();

    }

    private void configureGLSurfaceView() {

        setEGLContextClientVersion(2);
        setPreserveEGLContextOnPause(true);
        setEGLConfigChooser(new ConfigChooser(8, 8, 8, 0, 16, 8));

    }


}
