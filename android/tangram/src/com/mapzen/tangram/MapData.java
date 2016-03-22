package com.mapzen.tangram;

import com.mapzen.tangram.geometry.Geometry;
import com.mapzen.tangram.geometry.Point;
import com.mapzen.tangram.geometry.Polygon;
import com.mapzen.tangram.geometry.Polyline;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * MapData is a named, dynamic collection of drawable map features.
 */
public class MapData {

    String name;
    long pointer = 0;
    MapController map;
    List<String> geojson = new ArrayList<>();
    List<Geometry> features = new ArrayList<>();

    public MapData(String name) {
        this.name = name;
    }

    public String name() {
        return name;
    }

    public void addToMap(MapController map) {
        if (this.map != null) {
            removeFromMap();
        }
        this.map = map;
        this.pointer = map.nativeAddDataSource(name);
        syncWithMap();
    }

    public void removeFromMap() {
        map.nativeRemoveDataSource(pointer);
        pointer = 0;
        map = null;
    }

    public void syncWithMap() {
        map.nativeClearDataSource(pointer);
        for (String data : geojson) {
            map.nativeAddGeoJson(pointer, data);
        }
        for (Geometry geometry : features) {
            map.nativeAddFeature(pointer,
                    geometry.getCoordinateArray(),
                    geometry.getRingArray(),
                    geometry.getPropertyArray());
        }
    }

    public MapData addPoint(LngLat point, Map<String, String> properties) {
        features.add(new Point(point, properties));
        return this;
    }

    public MapData addPolyline(List<LngLat> polyline, Map<String, String> properties) {
        features.add(new Polyline(polyline, properties));
        return this;
    }

    public MapData addPolygon(List<List<LngLat>> polygon, Map<String, String> properties) {
        features.add(new Polygon(polygon, properties));
        return this;
    }

    public MapData addGeoJson(String data) {
        geojson.add(data);
        return this;
    }

    public MapData clear() {
        geojson.clear();
        features.clear();
        return this;
    }

}
