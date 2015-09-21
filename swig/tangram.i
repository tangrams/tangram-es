// swig -v -c++  -I../core/src -o ../android/tangram/jni/jniGenerated.cpp -outdir ../android/tangram/src/com/mapzen/tangram/ -package com.mapzen.tangram -java tangram.i

%module Tangram

%{
#include "tangram.h"
#include "data/properties.h"
#include "util/types.h"
#include <string>
#include <memory>
%}

%include "typemaps.i"
%include "std_common.i"
%include "std_string.i"
%include "std_vector.i"
%include "std_shared_ptr.i"

// http://www.swig.org/Doc3.0/Java.html#Java_imclass_pragmas
// Change class modifier of the native methods intermediate class (i.e. TangramJNI)
// %pragma(java) jniclassclassmodifiers="class"
// Change class modifier of the module class (i.e. Tangram)
// %pragma(java) moduleclassmodifiers="class"

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

// %rename (set) *::operator=;
// %rename (equals) *::operator==;

%rename (set) Tangram::LngLat::operator=;
%rename (equals) Tangram::LngLat::operator==;

// Extend LngLat on the java side, using setLngLat extension method
%typemap(javacode) Tangram::LngLat %{
    public LngLat set(double lng, double lat) {
         setLngLat(lng, lat);
         return this;
     }
%}
// Hide generated (extension) method in favor of this-returning java method
%javamethodmodifiers Tangram::LngLat::setLngLat "private"

// Include
// - LngLat struct as is,
// - ignore Range type
%ignore Tangram::Range;
%include "util/types.h"

// Extend on the native side - cannot return self here though
%extend Tangram::LngLat {
    void setLngLat(double lng, double lat) {
        $self->longitude = lng;
        $self->latitude = lat;
    }
}

// Create wrapper for Coordinates std::vector
%template(Coordinates) std::vector<Tangram::LngLat>;
// Add add() method for efficiency (without creating temporary LonLat)
%extend std::vector<Tangram::LngLat> {
    void add(double lng, double lat) {
        $self->push_back({lng, lat});
    }
}

%template(Polygon) std::vector<std::vector<Tangram::LngLat>>;

// Include external description for
// - Tags aka map<string,string>
// - DataSource
// - ClientGeoJsonSource
%include "jni_datasource.i"

namespace Tangram {
void addDataSource(std::shared_ptr<Tangram::DataSource> _source);
void clearDataSource(DataSource& _source, bool _data, bool _tiles);
}
