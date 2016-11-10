package com.mapzen.tangram;

import java.util.ArrayList;
import java.util.List;

/**
 * {@code TouchLabel} represents labels that can be selected on the scree
 */
public class TouchLabel {

    /**
     * Options for the type of TouchLabel
     */
    public enum LabelType {
        POINT,
        TEXT,
    }

    private List<LngLat> coordinates;
    private LabelType type;

    public TouchLabel(double[] coordinates, int type) {
        this.coordinates = new ArrayList<>();

        for (int i = 0; i < coordinates.length - 1; i += 2) {
            this.coordinates.add(new LngLat(coordinates[i], coordinates[i + 1]));
        }

        this.type = LabelType.values()[type];
    }

    public LabelType getType() {
        return this.type;
    }

    public List<LngLat> getCoordinates() {
        return this.coordinates;
    }
}
