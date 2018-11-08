package com.mapzen.tangram;

import android.support.annotation.Keep;

/**
 * Interface for a callback to receive information about labels picked from the map
 * Triggered after a call of {@link MapController#pickLabel(float, float)}
 * Listener should be set with {@link MapController#setLabelPickListener(LabelPickListener)}
 * The callback will be run on the main (UI) thread.
 */
@Keep
public interface LabelPickListener {
    /**
     * Receive information about labels found in a call to {@link MapController#pickLabel(float, float)}
     * @param labelPickResult The {@link LabelPickResult} that has been selected
     * @param positionX The horizontal screen coordinate of the picked location
     * @param positionY The vertical screen coordinate of the picked location
     */
    void onLabelPick(final LabelPickResult labelPickResult, final float positionX, final float positionY);
}
