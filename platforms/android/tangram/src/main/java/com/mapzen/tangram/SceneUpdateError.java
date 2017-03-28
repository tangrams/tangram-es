package com.mapzen.tangram;

import com.mapzen.tangram.MapController.Error;

/**
 * {@code SceneUpdateError} Holds an error status and its associated scene updated
 */

public class SceneUpdateError {


    private SceneUpdate sceneUpdate;
    private Error error;

    private SceneUpdateError(String sceneUpdatePath, String sceneUpdateValue, int error) {
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
