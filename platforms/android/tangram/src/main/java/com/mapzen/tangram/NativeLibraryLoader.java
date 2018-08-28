package com.mapzen.tangram;

public class NativeLibraryLoader {
    public static final boolean sNativeLibraryLoaded = loadLibrary();

    private static synchronized boolean loadLibrary() {
        try {
            System.loadLibrary("c++_shared");
            System.loadLibrary("tangram");
            return true;
        } catch (final Exception e) {
            android.util.Log.e("Tangram", "Failed to load native tangram libraries", e);
            return false;
        }
    }
}
