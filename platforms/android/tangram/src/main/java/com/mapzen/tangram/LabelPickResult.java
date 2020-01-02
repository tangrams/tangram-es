package com.mapzen.tangram;

import android.graphics.PointF;

import androidx.annotation.Keep;

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

    private Map<String, String> properties;
    private LngLat coordinates;
    private PointF screenPosition;
    private LabelType type;


    LabelPickResult(final Map<String, String> properties, final double longitude, final double latitude, final float screenX, final float screenY, final int type) {
        this.properties = properties;
        this.coordinates = new LngLat(longitude, latitude);
        this.screenPosition = new PointF(screenX, screenY);
        this.type = LabelType.values()[type];
    }

    /**
     * @return Properties of the picked feature, as string key-value pairs.
     */
    public Map<String, String> getProperties() {
        return this.properties;
    }

    /**
     * @return Geographic coordinates of the feature associated with this label.
     */
    public LngLat getCoordinates() {
        return this.coordinates;
    }

    /**
     * @return Screen position of the query that produced this result.
     */
    public PointF getScreenPosition() {
        return screenPosition;
    }

    /**
     * @return Type of this label.
     */
    public LabelType getType() {
        return this.type;
    }

}
