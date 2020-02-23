package com.mapzen.tangram;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

/**
 * Callback to receive information about features picked from the map, triggered after a call to
 * {@link MapController#pickFeature(float, float)}.
 * The listener is set with {@link MapController#setFeaturePickListener(FeaturePickListener)}.
 * The callback will be run on the main (UI) thread and is performed in the same order as
 * {@link MapController#pickFeature(float, float)} was called.
 */
@Keep
public interface FeaturePickListener {
    /**
     * Called when a feature pick query is complete, whether or not a feature was found.
     * @param result Info about the selected feature, or null if no feature was found.
     */
    void onFeaturePickComplete(@Nullable final FeaturePickResult result);
}
