package com.mapzen.tangram;

import com.mapzen.tangram.MapController.Error;

import androidx.annotation.Keep;

/**
 * {@code SceneError} Holds an error status and its associated scene updated
 */
@Keep
public class SceneError {


    private SceneUpdate sceneUpdate;
    private Error error;

    SceneError(final String sceneUpdatePath, final String sceneUpdateValue, final int error) {
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
