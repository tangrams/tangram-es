package com.mapzen.tangram;

import android.support.annotation.Keep;

import com.mapzen.tangram.MapController.Error;

/**
 * {@code SceneError} Holds an error status and its associated scene updated
 */
@Keep
public class SceneError {


    private SceneUpdate sceneUpdate;
    private Error error;

    private SceneError(final String sceneUpdatePath, final String sceneUpdateValue, final int error) {
        this.sceneUpdate = new SceneUpdate(sceneUpdatePath, sceneUpdateValue);
        this.error = Error.values()[error];
    }

    public SceneUpdate getSceneUpdate() {
        return this.sceneUpdate;
    }

    public Error getError() {
        return this.error;
    }
}
