%module tangram_api

%{
#define SWIG_FILE_WITH_INIT
#include "platform_linux.h"
#include "tangram.h"
#include "debug/textDisplay.h"
#include "log.h"
%}

/// Python style names
%rename("%(undercase)s", %$isfunction) "";
%rename("%(undercase)s", %$isvariable) "";
%rename("%(uppercase)s", %$isenumitem) "";


%include "properties.i"
%include "util_types.i"

// Wrapper for std::function callbacks, passing back swig types
//
// http://stackoverflow.com/questions/11516809/c-back-end-call-the-python
// -level-defined-callbacks-with-swig-wrapper/11522655#11522655
%define PY_CALLBACK(Callback,_TYPE,_SWIGTYPE)
%{
class Callback {
    PyObject *func;
    Callback& operator=(const Callback&); // Not allowed
public:
    Callback(const Callback& o) : func(o.func) {
      Py_XINCREF(func);
    }
    Callback(PyObject *func) : func(func) {
      Py_XINCREF(this->func);
      assert(PyCallable_Check(this->func));
    }
    ~Callback() { Py_XDECREF(func); }

    void operator()(_TYPE arg_in) const {
      if (!func || Py_None == func || !PyCallable_Check(func))
        return;
      PyObject *obj = SWIG_NewPointerObj(SWIG_as_voidptr(arg_in), _SWIGTYPE, 0);
      PyObject *args = PyTuple_New(1);
      PyTuple_SET_ITEM(args, 0, obj);
      PyObject *result = PyObject_Call(func, args, 0);
      Py_DECREF(args);
      Py_DECREF(obj);
      Py_XDECREF(result);
    }
};
%}
%enddef

PY_CALLBACK(PyFeaturePickCallback, const Tangram::FeaturePickResult*, SWIGTYPE_p_Tangram__FeaturePickResult)
PY_CALLBACK(PyLabelPickCallback, const Tangram::LabelPickResult*, SWIGTYPE_p_Tangram__LabelPickResult)

// Overwrite original callback-passing methods with our wrapping methods
%ignore Tangram::Map::pickFeatureAt(float, float, FeaturePickCallback);
%ignore Tangram::Map::pickLabelAt(float, float, LabelPickCallback);
%extend Tangram::Map {
    void pick_feature_at(float _x, float _y, PyObject *_callback) {
        $self->pickFeatureAt(_x, _y, PyFeaturePickCallback(_callback));
    }
    void pick_label_at(float _x, float _y, PyObject *_callback) {
        $self->pickLabelAt(_x, _y, PyLabelPickCallback(_callback));
    }
    void log_screen(const char* message) {
        LOGS("> %s", message);
    }
}

%ignore Tangram::LabelPickResult::FeaturePickResult;
%immutable Tangram::LabelPickResult::touchItem;
%immutable Tangram::LabelPickResult::type;
%immutable Tangram::LabelPickResult::coordinates;

%ignore Tangram::FeaturePickResult::FeaturePickResult;
%immutable Tangram::FeaturePickResult::properties;
%immutable Tangram::FeaturePickResult::position;

// Create bindings for these headers:
%include "tangram.h"
%include "debug/debugFlags.h"

%{
void poke_network_queue() {
    processNetworkQueue();
}

void drain_network_queue() {
    finishUrlRequests();
}
%}
void poke_network_queue();
void drain_network_queue();
