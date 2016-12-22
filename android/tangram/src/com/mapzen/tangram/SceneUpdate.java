package com.mapzen.tangram;

/**
 * {@code SceneUpdate} Represents a DataStructure to specify a yaml path and the corresponding value for a Scene Update.
 */

public class SceneUpdate {

    private String componentPath;
    private String componentValue;

    /**
     * Add a point feature to this collection.
     * @param path Series of yaml keys separated by a ".". Represents the scene path to be updated
     * @param value A yaml string which will update the value at the specified path
     */
    public SceneUpdate(String path, String value) {
        this.componentPath = path;
        this.componentValue = value;
    }

    public String getPath() { return componentPath; }
    public String getValue() { return componentValue; }

}
