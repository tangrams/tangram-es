%module DebugTangram 
%{
#include "debug.h"
%}

%include "enums.swg"
%javaconst(1);

%rename("%(uppercase)s") freeze_tiles;
%rename("%(uppercase)s") proxy_colors;
%rename("%(uppercase)s") tile_bounds;
%rename("%(uppercase)s") tile_infos;
%rename("%(uppercase)s") labels;

%include "debug.h"
