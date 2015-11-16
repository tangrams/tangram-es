%{
#include "tangram.h"
#include "data/dataSource.h"
#include "data/clientGeoJsonSource.h"
%}

%include "std_shared_ptr.i"

// Rename ClientGeoJsonSource to MapData on the Java side.
%rename(MapData) Tangram::ClientGeoJsonSource;
//%rename(getName) Tangram::DataSource::name;

%shared_ptr(Tangram::DataSource);
%shared_ptr(Tangram::ClientGeoJsonSource);

// Additional methods for DataSource
%typemap(javacode) Tangram::DataSource %{
    /**
     * Get the name of this data source
     * @return The name
     */
    public String getName() {
        return name();
    }
%}

// Additional imports for MapData
%typemap(javaimports) Tangram::ClientGeoJsonSource %{
import java.util.List;
%}
// Additional methods for MapData
%typemap(javacode) Tangram::ClientGeoJsonSource %{

    /**
     * Construct a new MapData object for adding drawable data to the map
     * @param name Name of the data source in the scene file for styling this
     *             object's data
     */
    public MapData(String name) {
        this(TangramJNI.new_MapData(name, ""), true);
    }

    /**
     * Remove all data from this source
     * @return This object, for chaining
     */
    public MapData clear() {
        clearJNI();
        return this;
    }

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
    public MapData addPoint(Properties props, LngLat point) {
        addPointJNI(props, point);
        return this;
    }

    /**
     * Add a line geometry to this data source
     * @param line List of LngLat points comprising the line
     * @return This object, for chaining
     */
    public MapData addLine(Properties props, List<LngLat> line) {
        // need to concatenate points
        Coordinates coords = new Coordinates();
        for (LngLat point : line) {
            coords.add(point);
        }
        addLineJNI(props, coords);
        return this;
    }

    public MapData addLine(Properties props, Coordinates line) {
        addLineJNI(props, line);
        return this;
    }

    /**
     * Add a polygon geometry to this data source
     * @param polygon List of lines of LngLat points, where each line represents
     *                a ring in the polygon as described in the GeoJSON spec
     * @return This object, for chaining
     */
    public MapData addPolygon(Properties props, List<List<LngLat>> polygon) {
        Polygon poly = new Polygon();

        // for (List<LngLat> ring : polygon) {
        //     Coordinates out = new Coordinates();
        //     for (LngLat point : ring) {
        //         out.add(point);
        //     }
        //     poly.add(out);
        // }

        // TODO add method to add empty ring and get handle to it.
        Coordinates dummy = new Coordinates();
        int rings = 0;

        for (List<LngLat> ring : polygon) {
            poly.add(dummy);

            Coordinates out = poly.get(rings);

            for (LngLat point : ring) {
                out.add(point);
            }
            rings++;
        }
        addPolyJNI(props, poly);
        return this;
    }

    public MapData addPolygon(Properties props, Polygon polygon) {
        addPolyJNI(props, polygon);
        return this;
    }
%}

// Hide generated (extension) methods in favor of this-returning java methods
%rename(addPointJNI) Tangram::ClientGeoJsonSource::addPoint;
%rename(addLineJNI) Tangram::ClientGeoJsonSource::addLine;
%rename(addPolyJNI) Tangram::ClientGeoJsonSource::addPoly;
%javamethodmodifiers Tangram::ClientGeoJsonSource::addPoint "private"
%javamethodmodifiers Tangram::ClientGeoJsonSource::addLine "private"
%javamethodmodifiers Tangram::ClientGeoJsonSource::addPoly "private"


namespace Tangram {

typedef std::vector<Tangram::LngLat> Coordinates;

class DataSource {
protected:
    DataSource(const std::string& _name, const std::string& _url);
    virtual void clearData() override;
};

class ClientGeoJsonSource : public DataSource {

public:
    ClientGeoJsonSource(const std::string& _name, const std::string& _url);
    void addData(const std::string& _data);
    void addPoint(const Properties& props, Tangram::LngLat point);
    void addLine(const Properties& props, const Coordinates& line);
    void addPoly(const Properties& props, const std::vector<Coordinates>& polygon);
};
} // namespace Tangram


%extend Tangram::DataSource {

    void update() {
        Tangram::clearDataSource(*($self), true, false);
    }
    void clearJNI() {
        Tangram::clearDataSource(*($self), true, true);
    }
    std::string name() {
        return $self->name();
    }
}
