package com.mapzen.tangram;

/**
 * {@code SceneUpdate} Represents a DataStructure to specify a yaml path and the corresponding value for a Scene Update.
 */

public class SceneUpdate {

    public String path;
    public String value;

    public SceneUpdate() { this("", ""); }

    public SceneUpdate(String path, String value) {
        set(path, value);
    }

    public SceneUpdate set(String p, String v) {
        path = p;
        value = v;
        return this;
    }

}
