package com.mapzen.tangram;

import android.support.annotation.Keep;

/**
 * Interface for a callback to receive the picked {@link Marker}
 * Triggered after a call of {@link MapController#pickMarker(float, float)}
 * Listener should be set with {@link MapController#setMarkerPickListener(MarkerPickListener)}
 * The callback will be run on the main (UI) thread.
 */
@Keep
public interface MarkerPickListener {
    /**
     * Receive information about marker found in a call to {@link MapController#pickMarker(float, float)}
     * @param markerPickResult The {@link MarkerPickResult} the marker that has been selected
     * @param positionX The horizontal screen coordinate of the picked location
     * @param positionY The vertical screen coordinate of the picked location
     */
    void onMarkerPick(final MarkerPickResult markerPickResult, final float positionX, final float positionY);
}
