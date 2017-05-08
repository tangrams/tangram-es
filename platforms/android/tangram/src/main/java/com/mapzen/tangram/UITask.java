package com.mapzen.tangram;

/**
 * {@code UITask} Task that picks up native code that will be run on UI thread.
 */

class UITask implements Runnable {

    private long mapPtr;

    UITask(long mapPtr) {
        this.mapPtr = mapPtr;
    }

    @Override
    public void run() {
        nativeExecutePendingUITasks(mapPtr);
    }

    private synchronized native void nativeExecutePendingUITasks(long mapPtr);
}
