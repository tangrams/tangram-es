package com.mapzen.tangram;

import android.util.Log;

class NativeLibraryLoader {
    static final boolean sNativeLibraryLoaded = loadLibrary();

    private static synchronized boolean loadLibrary() {
        try {
            System.loadLibrary("c++_shared");
            System.loadLibrary("tangram");
            return true;
        } catch (final SecurityException | UnsatisfiedLinkError | NullPointerException e) {
            Log.e(BuildConfig.TAG, "Failed to load native tangram libraries", e);
            return false;
        }
    }
}
