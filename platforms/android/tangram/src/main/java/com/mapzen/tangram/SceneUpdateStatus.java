package com.mapzen.tangram;

/**
 * Created by karim on 3/6/17.
 */

public class SceneUpdateStatus {

    public enum SceneUpdateError {
        PATH_NOT_FOUND,
        PATH_YAML_SYNTAX_ERROR,
        VALUE_YAML_SYNTAX_ERROR,
    }

    private SceneUpdate sceneUpdate;
    private SceneUpdateError error;

    private SceneUpdateStatus(String sceneUpdatePath, String sceneUpdateValue, int error) {
        this.sceneUpdate = new SceneUpdate(sceneUpdatePath, sceneUpdateValue);
        this.error = SceneUpdateError.values()[error];
    }

    public SceneUpdate getSceneUpdate() {
        return this.sceneUpdate;
    }

    public SceneUpdateError getError() {
        return this.error;
    }
}
