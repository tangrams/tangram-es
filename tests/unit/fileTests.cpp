#include "catch.hpp"

#include <iostream>
#include <sys/stat.h>
#include "tangram.h"
#include "platform.h"

TEST_CASE( "Compare byte size of allocated resource to os file size", "[Core][bytesFromResource]" ) {
#if 0
    unsigned int size;
    unsigned char* data = bytesFromFile("shaders/polygon.fs", PathType::internal, &size);

    // ask os for size
    struct stat st;
    stat("shaders/polygon.fs", &st);
    unsigned int sys_size = st.st_size;

    REQUIRE(sys_size == size);

    free(data);
#endif
}
