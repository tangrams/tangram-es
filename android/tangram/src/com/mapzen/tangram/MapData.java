package com.mapzen.tangram;

import java.util.List;

public class MapData {

    /**
     * Construct a new MapData object for adding drawable data to the map
     * @param name Name of the data source in the scene file for styling this object's data
     */
    public MapData(String name) {
        this.name = name;
        this.id = addDataSource(name);
    }

    /**
     * Get the name of this data source
     * @return The name
     */
    public String getName() {
        return name;
    }

    /**
     * Remove all data from this source
     * @return This object, for chaining
     */
    public MapData clear() {
        clearSourceData(id);
        return this;
    }

    /**
     * Add geometry from a GeoJSON string to this data source
     * @param data String of GeoJSON containing a Feature or FeatureCollection
     * @return This object, for chaining
     */
    public MapData addGeoJSON(String data) {
        addSourceGeoJSON(id, data);
        return this;
    }

    /**
     * Add a point geometry to this data source
     * @param point LngLat with the coordinates of the point
     * @return This object, for chaining
     */
    public MapData addPoint(LngLat point) {
        addSourcePoint(id, new double[]{ point.longitude, point.latitude });
        return this;
    }

    /**
     * Add a line geometry to this data source
     * @param line List of LngLat points comprising the line
     * @return This object, for chaining
     */
    public MapData addLine(List<LngLat> line) {
        // need to concatenate points
        double[] coords = new double[2 * line.size()];
        int i = 0;
        for (LngLat point : line) {
            coords[i++] = point.longitude;
            coords[i++] = point.latitude;
        }
        addSourceLine(id, coords, line.size());
        return this;
    }

    /**
     * Add a polygon geometry to this data source
     * @param polygon List of lines of LngLat points, where each line represents a ring in the
     *                polygon as described in the GeoJSON spec
     * @return This object, for chaining
     */
    public MapData addPolygon(List<List<LngLat>> polygon) {
        // need to concatenate points
        int n = 0, i = 0, j = 0;
        for (List<LngLat> ring : polygon) { n += ring.size(); }
        double[] coords = new double[2 * n];
        int[] ringLengths = new int[polygon.size()];
        for (List<LngLat> ring : polygon) {
            ringLengths[j++] = ring.size();
            for (LngLat point : ring) {
                coords[i++] = point.longitude;
                coords[i++] = point.latitude;
            }
        }
        addSourcePoly(id, coords, ringLengths, polygon.size());
        return this;
    }

    private String name;
    private int id;
    private synchronized native int addDataSource(String name);
    private synchronized native void clearSourceData(int id);
    private synchronized native void addSourceGeoJSON(int id, String data);
    private synchronized native void addSourcePoint(int id, double[] coords);
    private synchronized native void addSourceLine(int id, double[] coords, int length);
    private synchronized native void addSourcePoly(int id, double[] coords, int[] ringLengths, int rings);

}
