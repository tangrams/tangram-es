// swig -v -c++  -I../core/src -o ../android/tangram/jni/jniGenerated.cpp -outdir ../android/tangram/src/com/mapzen/tangram/ -package com.mapzen.tangram -java tangram.i

%module Tangram

%{
#include "data/properties.h"
#include <string>
#include <memory>
%}

// knows about things like int *OUTPUT:
%include "typemaps.i"
%include "std_common.i"
%include "std_string.i"

%include "std_shared_ptr.i"

%shared_ptr(Tangram::Properties);

namespace Tangram {
struct Properties {

    Properties() {}

    void clear() { props.clear(); }

    bool contains(const std::string& key) const;

    float getNumeric(const std::string& key) const;

    const std::string& getString(const std::string& key) const;
};
}
%extend Tangram::Properties {
    void add(std::string key, std::string value) {
        $self->add(key, value);
    }
    void add(std::string key, float value) {
        $self->add(key, value);
    }
}

// JNI Bindings for std::map<std::string,std::string>
%include "std_map.i"

namespace std {
%template(Tags) map<string, string>;
}

%{
#include "tangram.h"
#include "data/dataSource.h"
#include "data/clientGeoJsonSource.h"
%}

//%rename("%(lowercamelcase)s", %$isfunction) "";

// Rename ClientGeoJsonSource to MapData on the Java side.
%rename(MapData) Tangram::ClientGeoJsonSource;
//%rename(getName) Tangram::DataSource::name;

// http://www.swig.org/Doc3.0/Java.html#Java_imclass_pragmas
// Change class modifier of the native methods intermediate class (i.e. TangramJNI)
// %pragma(java) jniclassclassmodifiers="class"
// Change class modifier of the module class (i.e. Tangram)
// %pragma(java) moduleclassmodifiers="class"

%shared_ptr(Tangram::DataSource);
%shared_ptr(Tangram::ClientGeoJsonSource);

%typemap(javacode) Tangram::DataSource %{

    /**
     * Get the name of this data source
     * @return The name
     */
    public String getName() {
        return name();
    }
%}

%typemap(javaimports) Tangram::ClientGeoJsonSource %{
import java.util.List;
%}

%typemap(javacode) Tangram::ClientGeoJsonSource %{

    /**
     * Construct a new MapData object for adding drawable data to the map
     * @param name Name of the data source in the scene file for styling this object's data
     */
    public MapData(String name) {
        this(TangramJNI.new_MapData(name, ""), true);
    }

    // /**
    //  * Remove all data from this source
    //  * @return This object, for chaining
    //  */
    // public MapData clear() {
    //     //clearSourceData(id);
    //     return this;
    // }

    /**
     * Add geometry from a GeoJSON string to this data source
     * @param data String of GeoJSON containing a Feature or FeatureCollection
     * @return This object, for chaining
     */
    public MapData addGeoJSON(String data) {
        addData(data);
        return this;
    }

    /**
     * Add a point geometry to this data source
     * @param point LngLat with the coordinates of the point
     * @return This object, for chaining
     */
    public MapData addPoint(Tags tags, LngLat point) {
        addPoint(tags, new double[]{ point.longitude, point.latitude });
        return this;
    }

    /**
     * Add a line geometry to this data source
     * @param line List of LngLat points comprising the line
     * @return This object, for chaining
     */
    public MapData addLine(Tags tags, List<LngLat> line) {
        // need to concatenate points
        double[] coords = new double[2 * line.size()];
        int i = 0;
        for (LngLat point : line) {
            coords[i++] = point.longitude;
            coords[i++] = point.latitude;
        }
        addLine(tags, coords, line.size());
        return this;
    }

    /**
     * Add a polygon geometry to this data source
     * @param polygon List of lines of LngLat points, where each line represents a ring in the
     *                polygon as described in the GeoJSON spec
     * @return This object, for chaining
     */
    public MapData addPolygon(Tags tags, List<List<LngLat>> polygon) {
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
        addPoly(tags, coords, ringLengths, polygon.size());
        return this;
    }
%}

%include "array_nocpy.i"

namespace Tangram {

class DataSource {
protected:
    DataSource(const std::string& _name, const std::string& _url);
};

class ClientGeoJsonSource : public DataSource {

public:
    ClientGeoJsonSource(const std::string& _name, const std::string& _url);

    void addData(const std::string& _data);
    void addPoint(std::map<std::string,std::string> tags, double _coords[]);
    void addLine(std::map<std::string,std::string> tags, double _coords[], int _lineLength);
    void addPoly(std::map<std::string,std::string> tags, double _coords[], int _ringLengths[], int rings);

    virtual void clearData() override;

};
} // namespace

%extend Tangram::DataSource {

    void update() {
        Tangram::clearDataSource(*($self), false, true);
    }
    void clear() {
        Tangram::clearDataSource(*($self), true, true);
    }
    const std::string& name() {
         $self->name();
    }
}

namespace Tangram {
int addDataSource(std::shared_ptr<Tangram::DataSource> _source);
void clearDataSource(DataSource& _source, bool _data, bool _tiles);
}
