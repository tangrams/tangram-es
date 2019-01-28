package com.mapzen.tangram;

class NativeLibraryLoader {
    static final boolean sNativeLibraryLoaded = loadLibrary();

    private static synchronized boolean loadLibrary() {
        try {
            System.loadLibrary("c++_shared");
            System.loadLibrary("tangram");
            return true;
        } catch (final SecurityException | UnsatisfiedLinkError | NullPointerException e) {
            android.util.Log.e("Tangram", "Failed to load native tangram libraries", e);
            return false;
        }
    }
}
