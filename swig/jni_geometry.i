// %rename (set) *::operator=;
// %rename (equals) *::operator==;

%rename (set) Tangram::LngLat::operator=;
%rename (equals) Tangram::LngLat::operator==;

// Hide generated (extension) method in favor of this-returning java method
%javamethodmodifiers Tangram::LngLat::setLngLat "private"

// Create wrapper for Coordinates std::vector<LngLat>
// Extend LngLat on the java side, using setLngLat extension method +
// Docs: 25.10.11 Memory management when returning references to member variables
%typemap(javacode) Tangram::LngLat %{
  // Ensure that the GC doesn't collect any Polygon instance set from Java
  private Object owner;
  protected void addReference(Object obj) {
    owner = obj;
  }

  public LngLat set(double lng, double lat) {
    setLngLat(lng, lat);
    return this;
  }
%}
// Add a Java reference to prevent premature garbage collection and resulting use
// of dangling C++ pointer. Intended for methods that return pointers or
// references to a member variable.  In this case
%typemap(javaout) Tangram::LngLat& get {
    long cPtr = $jnicall;
    $javaclassname ret = null;
    if (cPtr != 0) {
      ret = new $javaclassname(cPtr, $owner);
      ret.addReference(this);
    }
    return ret;
}

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

%typemap(javacode) std::vector<Tangram::LngLat> %{
  // Ensure that the GC doesn't collect any Polygon instance set from Java
  private Object owner;
  protected void addReference(Object obj) {
    owner = obj;
  }
%}
%typemap(javaout) std::vector<Tangram::LngLat>& get {
    long cPtr = $jnicall;
    $javaclassname ret = null;
    if (cPtr != 0) {
      ret = new $javaclassname(cPtr, $owner);
      ret.addReference(this);
    }
    return ret;
}

// Create Coordinates Wrapper aka std::vector<LngLat>
%template(Coordinates) std::vector<Tangram::LngLat>;

// Add add() method for efficiency (without creating temporary LonLat)
%extend std::vector<Tangram::LngLat> {
    void add(double lng, double lat) {
        $self->push_back({lng, lat});
    }
}

// Create wrapper for Polygon aka std::vector<Coordinates>
%template(Polygon) std::vector<std::vector<Tangram::LngLat>>;
