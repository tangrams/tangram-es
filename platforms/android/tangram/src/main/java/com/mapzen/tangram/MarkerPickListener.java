package com.mapzen.tangram;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

/**
 * Interface for a callback to receive the picked {@link Marker}
 * Triggered after a call of {@link MapController#pickMarker(float, float)}
 * Listener should be set with {@link MapController#setMarkerPickListener(MarkerPickListener)}
 * The callback will be run on the main (UI) thread.
 */
@Keep
public interface MarkerPickListener {
    /**
     * Called when a marker pick query is complete, whether or not a marker was found.
     * @param result Info about the selected marker, or null if no marker was found.
     */
    void onMarkerPickComplete(@Nullable final MarkerPickResult result);
}
