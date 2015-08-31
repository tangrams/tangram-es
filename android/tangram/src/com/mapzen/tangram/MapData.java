package com.mapzen.tangram;

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
     * @param point Two-element array of longitude and latitude (in that order) in degrees
     * @return This object, for chaining
     */
    public MapData addPoint(double[] point) {
        addSourcePoint(id, point);
        return this;
    }

    /**
     * Add a line geometry to this data source
     * @param line Array of points, where points are as described in {@link #addPoint(double[])}
     * @return This object, for chaining
     */
    public MapData addLine(double[][] line) {
        // need to concatenate nested arrays
        double[] coords = new double[2 * line.length];
        int i = 0;
        for (double[] point : line) {
            coords[i++] = point[0];
            coords[i++] = point[1];
        }
        addSourceLine(id, coords, line.length);
        return this;
    }

    /**
     * Add a polygon geometry to this data source
     * @param polygon Array of lines, where lines are as described in {@link #addLine(double[][])};
     *                each line represents a ring in the polygon as specified in GeoJSON
     * @return This object, for chaining
     */
    public MapData addPolygon(double[][][] polygon) {
        // need to concatenate nested arrays
        int n = 0, i = 0, j = 0;
        for (double[][] ring : polygon) { n += ring.length; }
        double[] coords = new double[2 * n];
        int[] ringLengths = new int[polygon.length];
        for (double[][] ring : polygon) {
            ringLengths[j++] = ring.length;
            for (double[] point : ring) {
                coords[i++] = point[0];
                coords[i++] = point[1];
            }
        }
        addSourcePoly(id, coords, ringLengths, polygon.length);
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
