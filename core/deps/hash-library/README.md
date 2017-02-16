# hash-library - MD5

We are using the MD5 functionality found in Stephan Brumme's
C++ Hashing Library. 

http://create.stephan-brumme.com/hash-library/

MD5s are computed in one place in Tangram ES, mbtilesTileTask.cpp.
The raw data contents of a tile are hashed to create and MD5 hash.
This hash is a key between the map and images table in an MBTiles database.

## Git Repo

http://create.stephan-brumme.com/hash-library/.git

## Notes

I commented out the `#include <endian.h>` in md5.cpp, because
different platforms may or may not have this header. In fact,
Mac OS X has it as `#include<machine/endian.h>`. Without this,
md5.cpp assumes that we are operating as little endian. This should
be addressed if we plan on supporting a big endian machine.

http://create.stephan-brumme.com/disclaimer.html
