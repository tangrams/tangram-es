package com.mapzen.tangram;

import android.support.annotation.Keep;

@Keep
public interface MapChangeListener {
    /**
     * Called on the UI thread at the end of whenever the view is stationary, fully loaded, and
     * no camera animations are running.
     */
    void onViewComplete();

    /**
     * Called on the UI thread at the start of a camera animation or user interaction
     * @param animated false when camera updates are set immediately, true otherwise
     */
    void onRegionWillChange(boolean animated);

    /**
     * Called on the UI thread during a camera animation or when map is changing due to user interaction
     */
    void onRegionIsChanging();

    /**
     * Called on the UI thread at the end of a camera animation or user interaction
     * @param animated false when camera updates are set immediately, true otherwise
     */
    void onRegionDidChange(boolean animated);
}
