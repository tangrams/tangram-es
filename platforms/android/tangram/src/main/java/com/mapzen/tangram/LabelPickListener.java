package com.mapzen.tangram;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

/**
 * Callback to receive information about labels picked from the map, triggered after a call to
 * {@link MapController#pickLabel(float, float)}.
 * The listener is set with {@link MapController#setLabelPickListener(LabelPickListener)}
 * The callback will be run on the main (UI) thread and is performed in the same order as
 * {@link MapController#pickLabel(float, float)} was called.
 */
@Keep
public interface LabelPickListener {
    /**
     * Called when a label pick query is complete, whether or not a label was found.
     * @param result Info about the selected label, or null if no label was found.
     */
    void onLabelPickComplete(@Nullable final LabelPickResult result);
}
