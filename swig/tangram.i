// swig -v -c++  -I../core/src -o ../android/tangram/jni/jniGenerated.cpp -outdir ../android/tangram/src/com/mapzen/tangram/ -package com.mapzen.tangram -java tangram.i

%module Tangram

%{
#include "tangram.h"
#include "data/properties.h"
#include <string>
#include <memory>
%}

// knows about things like int *OUTPUT:
%include "typemaps.i"
%include "std_common.i"
%include "std_string.i"
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

%include "jni_datasource.i"

namespace Tangram {
int addDataSource(std::shared_ptr<Tangram::DataSource> _source);
void clearDataSource(DataSource& _source, bool _data, bool _tiles);
}
