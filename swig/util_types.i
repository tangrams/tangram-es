%{
#include "util/types.h"
%}

%rename("%(undercase)s", %$isfunction) "";
%rename("%(undercase)s", %$isvariable) "";
%rename("%(uppercase)s", %$isenumitem) "";

%rename(EASE) EaseType;

%ignore Tangram::Range;

%ignore Tangram::LngLat::operator=(LngLat&& _other);
%ignore Tangram::LngLat::operator=(const LngLat& _other);
%ignore Tangram::LngLat::LngLat(LngLat&& _other);

%include "util/types.h"
