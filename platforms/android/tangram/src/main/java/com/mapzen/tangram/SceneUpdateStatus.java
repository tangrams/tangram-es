package com.mapzen.tangram;

/**
 * {@code SceneUpdateStatus} Holds an error status and its associated scene updated
 */

public class SceneUpdateStatus {

    /**
     * Options representing an error generated after a scene update
     */
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
