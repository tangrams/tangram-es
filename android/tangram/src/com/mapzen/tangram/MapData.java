package com.mapzen.tangram;

public class MapData {

    private String name;
    private int id;
    private synchronized native int addDataSource(String name);
    private synchronized native void clearSourceData(int id);
    private synchronized native void addSourceGeoJSON(int id, String data);
    private synchronized native void addSourcePoint(int id, double[] coords);
    private synchronized native void addSourceLine(int id, double[] coords, int length);
    private synchronized native void addSourcePoly(int id, double[] coords, int[] ringLengths, int rings);

    public MapData(String name) {
        this.name = name;
        this.id = addDataSource(name);
    }

    public String getName() {
        return name;
    }

    public MapData clear() {
        clearSourceData(id);
        return this;
    }

    public MapData addGeoJSON(String data) {
        addSourceGeoJSON(id, data);
        return this;
    }

    public MapData addPoint(double[] point) {
        addSourcePoint(id, point);
        return this;
    }

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

}
