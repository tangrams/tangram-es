package com.mapzen.tangram;

import androidx.annotation.Keep;

@Keep
public interface MapChangeListener {
    /**
     * Called when the view becomes stationary, fully loaded, and has no camera animations running.
     *
     * This is always called on the UI thread.
     */
    void onViewComplete();

    /**
     * Called at the start of a map camera movement, either from touch input or an API call.
     *
     * When the movement is triggered by touch input, this will be called on the UI thread.
     * Otherwise, this will be called on the thread that invoked the camera movement.
     *
     * Camera changes using 'flyTo' and 'easing' are considered animation. Movement from touch input
     * and immediate camera changes are not considered animation. Movement from a 'fling' after a
     * gesture is considered an animation. A 'fling' produces separate 'WillChange' and 'DidChange'
     * callbacks from the gesture that precedes it.
     *
     * @param animated true when the movement is controlled by an animation, otherwise false.
     *
     */
    void onRegionWillChange(boolean animated);

    /**
     * Called repeatedly during a map camera movement, either from touch input or an API call.
     *
     * This is always called on the UI thread.
     */
    void onRegionIsChanging();

    /**
     * Called at the end of a map camera movement, either from touch input or an API call.
     *
     * This is always called on the UI thread.
     *
     * See {@link MapChangeListener#onRegionWillChange(boolean)}.
     *
     * @param animated true when the movement is controlled by an animation, otherwise false.
     */
    void onRegionDidChange(boolean animated);
}
