%include "jni_lnglat.i"

%typemap(javacode) std::vector<Tangram::LngLat> %{
  // Ensure that the GC doesn't collect any Polygon instance set from Java
  // While we holding a Coordinates Ring of it.
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
