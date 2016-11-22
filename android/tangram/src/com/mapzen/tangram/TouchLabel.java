package com.mapzen.tangram;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * {@code TouchLabel} represents labels that can be selected on the screen
 */
public class TouchLabel {

    /**
     * Options for the type of TouchLabel
     */
    public enum LabelType {
        ICON,
        TEXT,
    }

    private LngLat coordinate;
    private LabelType type;
    private Map<String, String> properties;

    public TouchLabel(double[] coordinates, int type, Map<String, String> properties) {
        this.properties = properties;
        this.coordinate = new LngLat(coordinates[0], coordinates[1]);
        this.type = LabelType.values()[type];
    }

    public LabelType getType() {
        return this.type;
    }

    /**
     * @return The coordinate of the feature for which this label has been created
     */
    public LngLat getCoordinate() {
        return this.coordinate;
    }

    /**
     * @return A mapping of string keys to string or number values
     */
    public Map<String, String> getProperties() {
        return this.properties;
    }
}
