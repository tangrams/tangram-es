/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the
 * License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS"
 * BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 */

package com.mapzen.tangram;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;

import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.EGLConfigChooser;
import android.util.Log;

/**
 * {@code ConfigChooser} is a convenience class for configuring a {@code GLSurfaceView}.
 */
public class ConfigChooser implements GLSurfaceView.EGLConfigChooser {

    /**
     * Construct a {@code ConfigChooser} to match the given framebuffer parameters
     * @param r Red bits
     * @param g Green bits
     * @param b Blue bits
     * @param a Alpha bits
     * @param depth Depth bits
     * @param stencil Stencil bits
     */
    public ConfigChooser (int r, int g, int b, int a, int depth, int stencil) {
        mRedSize = r;
        mGreenSize = g;
        mBlueSize = b;
        mAlphaSize = a;
        mDepthSize = depth;
        mStencilSize = stencil;
    }

    /*
     * This EGL config specification is used to specify 2.0 rendering. We use a minimum size of 4 bits for red/green/blue, but
     * will perform actual matching in chooseConfig() below.
     */
    private static int EGL_OPENGL_ES2_BIT = 4;
    private static int[] s_configAttribs = {EGL10.EGL_RED_SIZE, 4, EGL10.EGL_GREEN_SIZE, 4, EGL10.EGL_BLUE_SIZE, 4,
        EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL10.EGL_NONE};

    @Override
    public EGLConfig chooseConfig (EGL10 egl, EGLDisplay display) {

        // Get the number of minimally matching EGL configurations
        int[] num_config = new int[1];
        egl.eglChooseConfig(display, s_configAttribs, null, 0, num_config);

        int numConfigs = num_config[0];

        if (numConfigs <= 0) {
            throw new IllegalArgumentException("No configs match configSpec");
        }

        // Allocate then read the array of minimally matching EGL configs
        EGLConfig[] configs = new EGLConfig[numConfigs];
        egl.eglChooseConfig(display, s_configAttribs, configs, numConfigs, num_config);

        // Now return the "best" one
        return chooseConfig(egl, display, configs);
    }

    public EGLConfig chooseConfig (EGL10 egl, EGLDisplay display, EGLConfig[] configs) {

        EGLConfig bestConfig = null;
        int bestDepth = 0;
        int bestStencil = 0;

        for (EGLConfig config : configs) {
            int d = findConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0);
            int s = findConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);

            // We need at least mDepthSize and mStencilSize bits
            if (d < mDepthSize || s < mStencilSize) { continue; }

            // We want an *exact* match for red/green/blue/alpha
            int r = findConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0);
            int g = findConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
            int b = findConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
            int a = findConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);

            if (r == mRedSize && g == mGreenSize && b == mBlueSize && a == mAlphaSize &&
                d >= bestDepth && s >= bestStencil) {

                bestConfig = config;
                bestDepth = d;
                bestStencil = s;

            }
        }
        return bestConfig;
    }

    private int findConfigAttrib (EGL10 egl, EGLDisplay display, EGLConfig config, int attribute, int defaultValue) {

        if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
            return mValue[0];
        }
        return defaultValue;
    }

    // Subclasses can adjust these values:
    protected int mRedSize;
    protected int mGreenSize;
    protected int mBlueSize;
    protected int mAlphaSize;
    protected int mDepthSize;
    protected int mStencilSize;
    private int[] mValue = new int[1];
}
