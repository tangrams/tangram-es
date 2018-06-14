package com.mapzen.tangram;

import android.support.annotation.Keep;

import java.util.Map;

/**
 * {@code LabelPickResult} represents labels that can be selected on the screen
 */
@Keep
public class LabelPickResult {

    /**
     * Options for the type of LabelPickResult
     */
    public enum LabelType {
        ICON,
        TEXT,
    }

    private LngLat coordinates;
    private LabelType type;
    private Map<String, String> properties;

    private LabelPickResult(final double longitude, final double latitude, final int type,
                            final Map<String, String> properties) {
        this.properties = properties;
        this.coordinates = new LngLat(longitude, latitude);
        this.type = LabelType.values()[type];
    }

    public LabelType getType() {
        return this.type;
    }

    /**
     * @return The coordinate of the feature for which this label has been created
     */
    public LngLat getCoordinates() {
        return this.coordinates;
    }

    /**
     * @return A mapping of string keys to string or number values
     */
    public Map<String, String> getProperties() {
        return this.properties;
    }
}
