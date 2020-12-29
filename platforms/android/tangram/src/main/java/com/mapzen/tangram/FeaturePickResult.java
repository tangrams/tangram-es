package com.mapzen.tangram;

import android.graphics.PointF;

import java.util.Map;

public class FeaturePickResult {

    private final Map<String, String> properties;
    private final PointF screenPosition;

    FeaturePickResult(Map<String, String> properties, float screenX, float screenY) {
        this.properties = properties;
        this.screenPosition = new PointF(screenX, screenY);
    }

    /**
     * @return Properties of the picked feature, as string key-value pairs.
     */
    public Map<String, String> getProperties() {
        return properties;
    }

    /**
     * @return Screen position of the query that produced this result.
     */
    public PointF getScreenPosition() {
        return screenPosition;
    }
}
