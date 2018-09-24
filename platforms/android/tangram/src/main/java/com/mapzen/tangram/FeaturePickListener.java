package com.mapzen.tangram;

import android.support.annotation.Keep;

import java.util.Map;

/**
 * Interface for a callback to receive information about features picked from the map
 * Triggered after a call of {@link MapController#pickFeature(float, float)}
 * Listener should be set with {@link MapController#setFeaturePickListener(FeaturePickListener)}
 * The callback will be run on the main (UI) thread.
 */
@Keep
public interface FeaturePickListener {
    /**
     * Receive information about features found in a call to {@link MapController#pickFeature(float, float)}
     * @param properties A mapping of string keys to string or number values
     * @param positionX The horizontal screen coordinate of the picked location
     * @param positionY The vertical screen coordinate of the picked location
     */
    void onFeaturePick(final Map<String, String> properties, final float positionX, final float positionY);
}
