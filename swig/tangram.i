// swig -v -c++  -I../core/src -o ../android/tangram/jni/jniGenerated.cpp -outdir ../android/tangram/src/com/mapzen/tangram/ -package com.mapzen.tangram -java tangram.i

%module tangram

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
#include "data/dataSource.h"
#include "data/clientGeoJsonSource.h"
%}

%shared_ptr(Tangram::DataSource);
%shared_ptr(Tangram::ClientGeoJsonSource);

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
    void addPoint(double _coords[]);
    void addLine(double _coords[], int _lineLength);
    void addPoly(double _coords[], int _ringLengths[], int rings);

    virtual void clearData() override;

};
} // namespace

%{
#include "tangram.h"
%}

namespace Tangram {
int addDataSource(std::shared_ptr<Tangram::DataSource> _source);
}
